#include "dxdp-ipc/ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <grp.h>

void* t_ipc_srv_loop(void* arg);

ipc_srv* ipc_srv_init(const char* _name, packet_handler handler,
void* addl_data, const char* _group) {
    ipc_srv* srv = malloc(sizeof(ipc_srv));
    if (!srv) {
        return NULL;
    }

    srv->name = strdup(_name);
    if (!srv->name) {
        free(srv);
        return NULL;
    }

    size_t size = sizeof(mem_map);
    srv->enabled = false;
    srv->handler = handler;
    srv->addl_data = addl_data;

    shm_unlink(_name);

    mode_t old_umask = umask(0);

    srv->shm_fd = shm_open(_name, O_CREAT | O_RDWR | S_IRUSR | S_IWUSR, 0660);
    if (srv->shm_fd == -1) {
        free(srv->name);
        free(srv);
        return NULL;
    }

    umask(old_umask);

    struct group* grp = getgrnam(_group);
    if (!grp) {
        shm_unlink(_name);
        close(srv->shm_fd);
        free(srv->name);
        free(srv);
        return NULL;
    }

    if (fchown(srv->shm_fd, 0, grp->gr_gid) == -1) {
        shm_unlink(_name);
        close(srv->shm_fd);
        free(srv->name);
        free(srv);
        return NULL;
    }

    if (ftruncate(srv->shm_fd, size) == -1) {
        shm_unlink(_name);
        free(srv->name);
        close(srv->shm_fd);
        free(srv);
        return NULL;
    }

    srv->data = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, srv->shm_fd, 0);
    if (srv->data == MAP_FAILED) {
        shm_unlink(_name);
        free(srv->name);
        close(srv->shm_fd);
        free(srv);
        return NULL;
    }

    srv->enabled = true;

    for (size_t i = 0; i < MAX_QUEUE_PACKETS; i++) {
        atomic_store(&srv->data->free_map[i], false);
        atomic_store(&srv->data->cli_done_map[i], false);
        atomic_store(&srv->data->cli_done_reading_map[i], false);
        atomic_store(&srv->data->srv_done_map[i], false);
        srv->data->watchdog_map[i] = 0;
        srv->data->client_nonce[i] = 0;
    }

    if (pthread_create(&srv->thread, NULL, t_ipc_srv_loop, srv) != 0) {
        ipc_srv_destroy(srv);
        return NULL;
    }

    return srv;
}


void ipc_srv_destroy(ipc_srv* _srv) {
    size_t size = sizeof(mem_map);
    bool was_srv = false;

    if (_srv->enabled) {
        _srv->enabled = false;
            pthread_join(_srv->thread, NULL);
        was_srv = true;
    }

    munmap(_srv->data, size);
    close(_srv->shm_fd);

    if(was_srv) {
        shm_unlink(_srv->name);
    }

    free(_srv->name);
    free(_srv);
}


ipc_srv* ipc_cli_connect(const char* _name) {
    ipc_srv* cli = malloc(sizeof(ipc_srv));
    if (!cli) {
        return NULL;
    }

    size_t size = sizeof(mem_map);

    cli->name = strdup(_name);
    if (!cli->name) {
        free(cli);
        return NULL;
    }

    cli->enabled = false;

    cli->shm_fd = shm_open(_name, O_RDWR, 0666);
    if (cli->shm_fd == -1) {
        free(cli->name);
        free(cli);
        return NULL;
    }

    cli->data = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, cli->shm_fd, 0);
    if (cli->data == MAP_FAILED) {
        free(cli->name);
        close(cli->shm_fd);
        free(cli);
        return NULL;
    }

    return cli;
}

bool ipc_cli_send(ipc_srv* client, ipc_packet* _packet, ipc_packet* _response) {
    mem_map* data = client->data;
    uint32_t nonce = (uint32_t)rand();
    size_t index = MAX_QUEUE_PACKETS;

    // Find free index
    for (size_t i = 0; i < MAX_QUEUE_PACKETS; i++) {
        bool expected = false;
        if (atomic_compare_exchange_strong(&data->free_map[i], &expected, true)) {
            index = i;
            break;
        }
    }
    if (index == MAX_QUEUE_PACKETS) {
        return false;
    }

    // Set nonce
    data->client_nonce[index] = nonce;

    // Set packet
    data->packet_in[index] = *_packet;

    // Set done writing bit
    atomic_store(&data->cli_done_map[index], true);

    // Start waiting for response
    time_t start_time = time(NULL);
    while (!atomic_load(&data->srv_done_map[index])) {
        if (time(NULL) - start_time > SERVER_TIMEOUT) {
            fprintf(stderr, "Server response timed out.\n");
            atomic_store(&data->free_map[index], false);
            atomic_store(&data->cli_done_map[index], false);
            data->client_nonce[index] = 0;
            return false;
        }
        usleep(1000);
    }

    // Verify nonce to ensure the slot hasn't been reused
    if (data->client_nonce[index] != nonce) {
        atomic_store(&data->free_map[index], false);
        atomic_store(&data->cli_done_map[index], false);
        atomic_store(&data->srv_done_map[index], false);
        data->client_nonce[index] = 0;
        return false;
    }

    // Read response packet
    *_response = data->packet_response[index];

    // Set done reading bit
    atomic_store(&data->cli_done_reading_map[index], true);

    return true;
}

void* t_ipc_srv_loop(void* arg) {
    ipc_srv* server = (ipc_srv*)arg;
    mem_map* data = server->data;

    while (server->enabled) {
        for (size_t i = 0; i < MAX_QUEUE_PACKETS; i++) {

            // Find unfree index
            if (atomic_load(&data->free_map[i])) {
                // Check if client done writing, and, server has no touched it yet
                if (atomic_load(&data->cli_done_map[i])
                && !atomic_load(&data->srv_done_map[i])) {
                    // Prepare handler context
                    handler_context context;
                    context.response_packet = &data->packet_response[i];
                    context.addl_data = server->addl_data;
                    context.srv = server;

                    // Invoke the handler with client_packet and context
                    server->handler(&data->packet_in[i], &context);

                    // Set srv_done_map to indicate server has processed the packet
                    atomic_store(&data->srv_done_map[i], true);

                    // Update the watchdog timestamp
                    data->watchdog_map[i] = time(NULL);
                }

                /* If both client and server have completed
                 * their operations, clean up the slot
                 */
                if (atomic_load(&data->cli_done_map[i])
                && atomic_load(&data->srv_done_map[i])) {
                    // Check if client has read the response
                    if (atomic_load(&data->cli_done_reading_map[i])) {
                        // Reset all maps and data for this slot
                        memset(&data->packet_in[i], 0, sizeof(ipc_packet));
                        memset(&data->packet_response[i], 0, sizeof(ipc_packet));
                        data->watchdog_map[i] = 0;
                        data->client_nonce[i] = 0;

                        atomic_store(&data->cli_done_map[i], false);
                        atomic_store(&data->cli_done_reading_map[i], false);
                        atomic_store(&data->srv_done_map[i], false);
                        atomic_store(&data->free_map[i], false);
                    }
                }

                /* Watchdog: Check if the server has been waiting
                 * too long for the client to read the response
                 */
                if (atomic_load(&data->srv_done_map[i])
                && !atomic_load(&data->cli_done_reading_map[i])) {
                    time_t current_time = time(NULL);
                    if (current_time - data->watchdog_map[i] > SERVER_TIMEOUT) {

                        // Reset all maps and data for this slot
                        memset(&data->packet_in[i], 0, sizeof(ipc_packet));
                        memset(&data->packet_response[i], 0, sizeof(ipc_packet));
                        data->watchdog_map[i] = 0;
                        data->client_nonce[i] = 0;

                        atomic_store(&data->cli_done_map[i], false);
                        atomic_store(&data->cli_done_reading_map[i], false);
                        atomic_store(&data->srv_done_map[i], false);
                        atomic_store(&data->free_map[i], false);
                    }
                }
            }
        }

        usleep(1000);
    }

    return NULL;
}

ipc_packet* ipc_new_packet() {
    ipc_packet* tmp = malloc(sizeof(ipc_packet));
    if(!tmp) {
        return NULL;
    }

    return tmp;
}
#ifndef SYSUTIL_H_
#define SYSUTIL_H_

#define DEFAULT_CONFIG_PATH "/etc/dxdp/dxdp.conf"
#define DEFAULT_COMPILER_PATH "/usr/bin/dxdp-compiler"
#define DEFAULT_CDB_PATH "/lib/dxdp/cli.db"
#define DEFAULT_PID_PATH "/var/run/dxdpd/dxdpd.pid"
#define IPC_SHM_NAME "dxdp-ipc"
#define IPC_GRP_NAME "dxdp-ipcg"
#define IPC_SHM_CIMP_NAME "/dxdp-compiler"

bool sys_is_already_running();

void sys_shutdown();

bool is_root();

#endif // SYSUTIL_H_
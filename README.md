# dXDP

## About

dXDP is a high-performance, low-level kernel application leveraging XDP/eBPF to provide dynamic control over incoming packets on a machine. Filter IP packets by whitelisting or blacklisting IP addresses at the network interface level. Designed for flexibility and efficiency, dXDP incorporates a custom Just-In-Time (JIT) compiler and supports IP geolocation filtering using an easy-to-update database.

Common uses for dXDP includes providing a last-resort endpoint security firewall for servers, a robust firewall for servers which cannot use hardware firewalls, or lastly a 
method to increase security post hardware firewall without adding significant
delay to the network stack.

<br/>

## Key Features
##### eBPF Powered
- Utilizes the Extended Berkeley Packet Filter technology within the Linux kernel to provide a highly efficient mechanism for IP traffic management within the userspace, allowing for rapid packet processing directly at the kernel level without additional overhead.

##### Just-In-Time (JIT) Compilation
- Integrates a custom built Clang-based JIT compiler that compiles eBPF programs directly in memory, optimizing the execution speed and immediacy of IP filtering rules application.

##### Advanced Data Structures
- Optimized for computational efficiency while keeping memory usage relatively tame by using proper algorithms where necessary, such as an interval red black binary search tree for address querying when packets arrive on the interface.

##### Interface-Level Filtering: 
- Provides the ability to apply filters at different network interface levels, allowing for precise control over internet traffic flows across various network segments.

##### Geolocation Filtering
- Integrates advanced geolocation-based IP filtering, allowing for the dynamic blocking or unblocking of traffic based on geographic origin.

##### Command-Line Interface:
- Intuitive CLI for easy management, automation, and ease of AI pipeline implementation.

##### Security:
- Built for internet servers and thus built with security in mind.

<br/>

## Getting Started
##### 1. Downloading daemon and CLI driver

##### 2. Build and Install

##### 3. Edit configuration file (see [wiki/config](https://github.com/rackd/dxdp/wiki/How-To-Edit-Config-File))

##### 4. Start daemon

<br/>

## Build from Source
```
git clone https://github.com/rackd/dxdp.git
cd dxdp
make build
sudo make install
```

<br/>

## Usage
There are three ways to control filtering.
1. Statically, via file databases and configuration file. (see wiki/usage#i_want_to):
2. Dynamically, via the command line (see [wiki/how-do-i](https://github.com/rackd/dxdp/wiki/how-do-i))
3. <s>Dynamically, via the C++ or Python API. (see [wiki/how-do-i](https://github.com/rackd/dxdp/wiki/how-do-i))</s> (Coming soon)

**Note: you must edit the configuration file before starting dxdp:**
```c
sudo cp /etc/dxdp/sample.conf /etc/dxdp/dxdp.conf
vim /etc/dxdp/dxdp.conf
```
<br/>

## CLI Interface Examples
1. ```dxdp update -s BannedIPS -r ff06::c3```
2. ```dxdp update -s Section2 -r 192.168.1.12-192.168.1.24```
3. ```dxdp update -s Blocked -af ~/my_ips/new-bad-list.txt```

<br/>

## Options.
dXDP options are handled via enviormental variable. Currently, you change the
following options with the following variables:
1. **CONFIG_PATH** (path to config)
    - By default dXDP looks for config files at /etc/dxdp/dxdp.conf

2. **CLI_DB_PATH** (path to cli db)
    - Default: /var/dxdp/cli.db

3. **COMP_PATH** (path to dXDP compiler)
    - Default: /usr/bin/dxdp-compiler

4. **LOG_FILE_PATH** (path to log file)
    - If this variable is defined, then, dXDP will enable logging to a file at
    this path

<br/>

## Performance metrics 
<s>(see wiki/performance)</s> (Coming soon)

<br/>

## Roadmap
- Expanding beyond the IP protocol.
- Implement more algorithms with self testing latency... allowing for even more efficiency.
- Windows port
- Performance stats
- API for programmatic access

<br/>

# Real Experiment for Recovery Policies

### External Sender Configuration

To allow the sender to reach the receiver network through the router, a static route is configured.

**On Linux:**

```
sudo ip route add <RECEIVER_NET>/24 via <ROUTER_IP>
```

**On Windows:**

```
route add <RECEIVER_NET> mask 255.255.255.0 <ROUTER_IP> -p
```

### Routing Configuration

IP forwarding is enabled on the router node:

```
sysctl net.ipv4.ip_forward=1
```

### Network Impairments

Packet delay and loss are introduced **artificially** using the `netem` traffic control module:

```
tc qdisc replace dev veth0 root netem delay <DELAY> loss <LOSS>%
```

This allows controlled experimentation under different network conditions.

### Receiver (`rx`) – Linux Network Namespace

The receiver is implemented as an isolated Linux network namespace in order to:

- avoid interference with the host network stack
- simulate a clean and independent endpoint

#### Namespace Creation and Interface Setup

```
ip netns add rx
ip link add veth0 type veth peer name veth1
ip link set veth1 netns rx
```

#### IP Addressing and Interface Activation

```
ip addr add <ROUTER_NET_IP>/24 dev veth0
ip link set veth0 up

ip netns exec rx ip addr add <RECEIVER_IP>/24 dev veth1
ip netns exec rx ip link set veth1 up
ip netns exec rx ip link set lo up
```

#### Default Route Inside the Namespace

```
ip netns exec rx ip route add default via <ROUTER_NET_IP>
```

### Application Execution

The server application is executed inside the `rx` namespace:

```
ip netns exec rx <SERVER_APP>
```

### Packet Capture

Traffic is captured on the sender interface for offline analysis:

```
tcpdump -i <SENDER_IFACE> -w sender_<scenario>.pcap tcp
```

### Offline Analysis

Captured traces are analyzed using `tcpstat`:

```
tcpstat -r sender_<scenario>.pcap 0.5 -o "%R\t%T\n"
```

### Sender Configuration

**Linux:**

```
iperf3 -c <RECEIVER_IP> -p <PORT> -t <DURATION_SEC> -i 1
```

**Windows:**

```
.\iperf3.exe -c <RECEIVER_IP> -p <PORT> -t <DURATION_SEC> -i 1
```



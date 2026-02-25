# WNMA-Tcp-experiments
Student handbook with the source code for each experiment

## Ns3 installation

```
#Install Dependencies (Ubuntu)
sudo apt update
sudo apt install -y build-essential cmake ninja-build git python3
#Download ns-3
git clone https://gitlab.com/nsnam/ns-3-dev.git
cd ns-3-dev
Configure and Build
./ns3 configure
./ns3 build
```

## Experiment A1 – Fairness Evaluation
In this experiment, fairness is intended as **intra-protocol fairness**, namely the ability of a TCP algorithm to evenly distribute the available bandwidth among multiple flows that use the same congestion control algorithm.

For execution `./ns3 run "scratch/tcp-fairness --tcp=BBR"` or `./ns3 run "scratch/tcp-fairness --tcp=CUBIC"`

## Experiment A2 – Inter-Protocol Friendliness

In this experiment, we evaluate **friendliness**, a metric used to assess how two different TCP algorithms coexist when sharing the same network resource.

In this case, the focus is not on fairness among identical flows, but rather on the **mutual behavior** between different protocols.

For execution `./ns3 run scratch/tcp-friendliness`

## Experiment B – Starbucks Scenario (Bufferbloat)
**What is the Starbucks scenario?**
The Starbucks scenario represents a situation where many users share a slow network with very large
buffers, such as a public Wi-Fi connection.
The typical issue is not limited bandwidth, but rather the explosion of latency when buffers become full
(bufferbloat).
The objective of this experiment is to understand how a TCP algorithm behaves when the network is
characterized by very large queues.
In particular, we aim to evaluate whether the algorithm tends to fill the queue or instead attempts to limit
delay.

For execution `./ns3 run "scratch/tcp-starbucks --tcp=BBR"` or `./ns3 run "scratch/tcp-starbucks --tcp=CUBIC"`

## Experiment C – Mobility Impact
In this experiment, we aim to evaluate how TCP reacts to network changes, specifically variations in RTT.

For execution `./ns3 run "scratch/tcp-mobility --tcp=BBR"` or  `./ns3 run "scratch/tcp-mobility --tcp=CUBIC"`

## Experiment D – Download Performance

The objective of the Download Performance experiment is to evaluate the pure performance of TCP under ideal conditions, without external factors that could influence the algorithm’s behavior.

In particular, we aim to measure:

- how quickly a file is downloaded
- how efficiently TCP utilizes the available bandwidth

For execution  `./ns3 run "scratch/tcp-download --tcp=BBR"` or `./ns3 run "scratch/tcp-download --tcp=CUBIC"`



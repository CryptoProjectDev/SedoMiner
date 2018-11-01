## SEDO-miner

![SEDO_small](http://sedocoin.org/wp-content/uploads/2018/10/logo_blue_240.png)

[Deployed on ROPSTEN Ethereum](https://ropsten.etherscan.io/address/0x3c3f4afc4ae44a5486dfd5cdc1712fada97fbea4)

[Deployed on MAINNET Ethereum](https://etherscan.io/address/0x0f00f1696218eaefa2d2330df3d6d1f94813b38f)

This page describes the miner for **SEDO coin**.  For the regular ethereum miner, [click here](https://github.com/mining-visualizer/MVis-ethminer).

This is a fork of MVis-tokenminer program https://github.com/mining-visualizer/MVis-tokenminer (you can use both SEDO miner and MVis token miner to mine SEDO coin, 0xbitcoin and all other compatible tokens)

* This miner should work with any GPU that supports OpenCL, ie. pretty much all AMDs and most NVidia.
* Windows binaries can be downloaded from the  [Releases](https://github.com/CryptoProjectDev/SedoMiner/releases) page, or you can build from source (see below).
* For Linux, the only option at present is to build from source.  See the instructions below.  
* This miner supports both pool mining and solo mining. If you want to mine solo, you either need to run your own node, or use a public one like the ones Infura provides.
* When in pool mining mode, a 1.0% dev fee is in effect. Every 4 hours it switches to 'dev fee mining' for a short period of time, based on the percent.


### Installation

* Unzip the [download package]() anywhere you like.  
* Open up `SEDOminer.ini` using any text editor and set the following configuration items:
* For **Pool Mining**, you need to specify your mining pool and your ETH address. 

```
[Node]
Host=http://your_mining_pool.com   
RPCPort=8080
...
[SEDO]
EthAddress=YOUR_ETH_ADDRESS
```

* For **SOLO Mining**, you need to specify your mining pool and your ETH address. 
```
[Node]
Host=https://mainnet.infura.io/f12d6274997840158b99b418f0ed8ec1f12d6274997840158b99b418f0ed8ec1
RPCPort=8545
...
[SEDO]
EthAddress=YOUR_ETH_ADDRESS
EthAddressPrivateKey=YOUR_ETH_ADDRESS_PRIVATE_KEY
TokenContract=0x0F00f1696218EaeFa2D2330Df3D6D1f94813b38f 
```
* All other settings in the `[SEDO]` section can be left as is or can be optimized.
* You can also specify the pool mining address on the command line (-N).  See below for all command line options.
* If your mining pool supports the **stratum protocol**, change the `RPCPort=8586` line to `StratumPort=9192`.  Consult with your mining pool for the actual port # to use.
* For **Solo Mining**:
    * Input an ETH account and associated private key. 
    * You can specify the address and port of your node in the `.ini` file, or on the command line.
    * You can enable gas price bidding.  (see comments in the file).  Note that enabling this feature does not guarantee that you will win every bid.  Network latency will sometimes result in failed transactions, even if you 'out-bid' the other transaction.
* You can leave the .INI file in the executable folder,  or you can move it to `C:\Users\[USER]\AppData\Local\SedoMiner` on Windows, or `$HOME/.config/SedoMiner` on Linux.  If that folder path does not exist, you will need to create it manually. If for some reason that file exists at both locations, the one in the executable folder will take precedence. 
* **WINDOWS ONLY**: download and install both the [VC 2013 Redistributable](https://www.microsoft.com/en-ca/download/details.aspx?id=40784) and the [VC 2015 Redistributable](https://www.microsoft.com/en-ca/download/details.aspx?id=48145)
* Double-click on the file `list-devices.bat`.  Examine the screen output and verify your GPU's are recognized.  Pay special attention to the PlatformID.  If it is anything other than 0, you will need to edit the `start-mining.bat` file and change the `--opencl-platform <n>` argument.
* Start POOL MINING by double-clicking on `start-mining.bat`.
* Start SOLO MINING with `SedoMiner.exe -S -G`.  This assumes you've specified the node address in the .INI file.
* **COOLING**: Please note that SEDO-miner does not have any features to set fan speeds or regulate cooling, other than shutting down if things get too hot.  Usually the AMD drivers do a pretty good job in that regard, but sometimes they don't.  It is your responsibility to monitor your fan speeds and GPU temperatures. If the AMD drivers aren't setting fan speeds high enough, you may need to use a 3rd part product,  like Speedfan or Afterburner.

#### Configuration Details ####

SEDO-miner is partially configured via command line parameters, and partially by settings in `Sedominer.ini`.  Run `Sedominer --help` to see which settings are available on the command line.  Have a look inside the .ini file to see what settings can be configured there. (It is fairly well commented).  Some settings can *only* be set on the command line (legacy ones mostly), some settings can *only* be set in the .ini file (newer ones mostly), and some can be set in both.  For the last group, command line settings take precedence over the .ini file settings.

#### Command Line Options ####

```
Node configuration:
    -N, --node <host:rpc_port>  Host address and RPC port of your node/mining pool. (default: 127.0.0.1:8545)
    -N2, --node2 <host:rpc_port>  Failover node/mining pool (default: disabled)
    -I, --polling-interval <n>  Check for new work every <n> milliseconds (default: 2000). 
    -R, --farm-retries <n> Number of retries until switch to failover (default: 4)

 Benchmarking mode:
    -M,--benchmark  Benchmark for mining and exit
    --benchmark-warmup <seconds>  Set the duration of warmup for the benchmark tests (default: 8).
    --benchmark-trial <seconds>  Set the duration for each trial for the benchmark tests (default: 3).
    --benchmark-trials <n>  Set the number of benchmark tests (default: 5).

 Mining configuration:
    -P  Pool mining
    -S  Solo mining
    -C,--cpu  CPU mining
    -G,--opencl  When mining use the GPU via OpenCL.
    --cl-local-work <n> Set the OpenCL local work size. Default is 128
    --cl-work-multiplier <n> This value multiplied by the cl-local-work value equals the number of hashes computed per kernel 
       run (ie. global work size). (Default: 8192)
    --opencl-platform <n>  When mining using -G/--opencl use OpenCL platform n (default: 0).
    --opencl-device <n>  When mining using -G/--opencl use OpenCL device n (default: 0).
    --opencl-devices <0 1 ..n> Select which OpenCL devices to mine on. Default is to use all
    -t, --mining-threads <n> Limit number of CPU miners to n (default: use everything available on selected platform)
    --allow-opencl-cpu  Allows CPU to be considered as an OpenCL device if the OpenCL platform supports it.
    --list-devices List the detected OpenCL/CUDA devices and exit. Should be combined with -G or -U flag
    --cl-extragpu-mem <n> Set the memory (in MB) you believe your GPU requires for stuff other than mining. default: 0

 Miscellaneous Options:
    --config <FileSpec>  - Full path to an INI file containing program options. Default location is 1) the executable folder, or 
                           if not there, then in 2) %LocalAppData%/SedoMiner/SedoMiner.ini (Windows) or 
                           $HOME/.config/SedoMiner/SedoMiner.ini (Linux).  If this option is specified,  it must appear 
                           before all others.

 General Options:
    -V,--version  Show the version and exit.
    -h,--help  Show this help message and exit.
```

# LFRU-Cache-Replacement-Policy-simulator
The cache simulator, a modified version of Levindo Gabriel Taschetto Neto's 
LRU&FIFO Cache Implementation, is a program that simulate LFRU cache replacement policy.

# Specification File Content :
* line size
* Number of line
* Associativity

The specification file will control how the simulator will simulate the blocks
	sets = (Number of line)/(Associativity)
	blocks per line = associativity
	privileged blocks = associativity/2
	
	Example :
		line size = 64
		number of lines = 512
		associativity = 8
	this data would give 64 sets with each 8 blocks (4 privileged blocks)
	
# Cache Structure
Cache is a struct that contains five information:
* Data_Cache // 1 when there's data, and 0 when there's none
* Upper // Data line (tag)
* T_Access (used in the LRU algorithm) // Last accessed time, using a counter clock
* Hit_Frequency (used in the LFU algorithm) // How many time the data accessed
* Trait // 1 for privileged, and 0 for unprivileged

# How to use
```Terminal
"compiled_executable.exe" "Tests/Cache_Descriptions/cachedescriptionfile.format" "Tests/Trace_Files/inputfile.format" "Results/outputfile.format"
```
without "s

# Citation
M. Bilal and S. -G. Kang, "A Cache Management Scheme for Efficient Content Eviction and Replication in Cache Networks," in IEEE Access, vol. 5, pp. 1692-1701, 2017, doi: 10.1109/ACCESS.2017.2669344.
Paper : https://arxiv.org/ftp/arxiv/papers/1702/1702.04078.pdf
Related Patent : https://patentimages.storage.googleapis.com/60/c5/34/c94ab8b27e2f9d/US10819823.pdf

## License

MIT License. Click [here](LICENSE.md) for more information about this license.

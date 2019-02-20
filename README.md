# Distributed Runtime system (dRTS) for Papyrus-RT

This repositoy contains feature patches for Papyrus-RT 1.0 to enable the development of distributed applications.

## Installation
The plugins can be installed from the Eclipse update site http://papyrusrt.jahed.ca/distribution

## Usage
To start, follow the usual Papyrus-RT workflow. After generating the code, the project must be build using Cmake. This ensure that all the libraries dependencies are built.
```
cd <src_dir>
mkdir build && cd build
cmake ..
make
```
The generated binary can be executed in 3 modes.
- Executing the binary without any argument will run the program in Single process mode. This is equivelant to the execution provided by vanilla Papyrus-RT.
- Executing the binary with the -i and -c arguments will launch the program in Distributed Master mode. In a distributed setup, exacly one process must be execute in this mode.
- Executing the binary with the -i argument will launch the program in Distributed Client mode. 
```
# Single process mode
./TopMain
# Distributed Master mode
./TopMain -i tcp://127.0.0.1:5555 -c deployment.json
# Distributed Client mode
./TopMain -i tcp://127.0.0.1:4444
```
The ```-c``` argument is used to specify a Capsules' deployment map. The deployment map is a JSON file that maps each Capsule in the application to a Controller (thread), and each Controller to a Host. The file has the following syntax:
```
{
	"hosts": [{
		"name": "host1",
		"address": "tcp://127.0.0.1:5555"
	}],
	"controllers": [{
		"name": "controller1",
		"host": "host1"
	}],
	"Capsules": [{
		"name": "Top.Capsule1",
		"controller": "controller1"
	}]
}
```


## Development
Simply import all projects whithin the repository into Papyrus-RT. The only other dependncy that must be installed is [Xtext](https://www.eclipse.org/Xtext/).

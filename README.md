# Coconut tool 
This Typestate Library is a tool that enables C++ users to define a protocol for their classes. This tool will significantly facilitate and secure the user’s work by detecting any violations of the protocol that the user specifies for objects.


### Download C++ ###
- On Windows, you can download it from [here.](https://docs.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=msvc-170/ "Install C and C++ support in Visual Studio") 

- On Mac, you can download the Xcode program from the Apple store, use Command Line Tool, and choose C++ in the Language section.

- On Linux, you can download it from [here.](https://learn.microsoft.com/en-us/cpp/linux/download-install-and-setup-the-linux-development-workload?view=msvc-170) 


  
### Download Coconut file from here to try it with docker and experiment 

(https://doi.org/10.5281/zenodo.14478714)

### Download Cocount ###

```
git clone https://github.com/CoLab-Glasgow/Coconut.git

```



#### Install Dependencies
- Boost Library (Version 1.81.0):
  ```shell
  git clone --branch boost-1.81.0 --depth 1 https://github.com/boostorg/boost.git
  ```


### Environment Options
You can run these commands and compile the project using:
- **WSL (Windows Subsystem for Linux)**: Provides a Linux-like environment on Windows. https://learn.microsoft.com/en-us/windows/wsl/install 
- **Cygwin**: Offers a POSIX-compatible environment for Windows. https://www.cygwin.com/install.html 


### GCC and Related Tools
The project uses **GCC 13**, which includes:
- `gcc-13` - GNU Compiler Collection (C Compiler)
- `g++-13` - GNU Compiler Collection (C++ Compiler)
- `gcc-13-plugin-dev` - Plugin Development Tools for GCC 13

inside wsl or Cygwin

```shell
sudo apt update
sudo apt install -y gcc-13 g++-13 gcc-13-plugin-dev

```


#### Build the Coconut Library
1. **Navigate to the "Coconut" directory**:
   ```shell
   cd Coconut
   ```
2. **Create a build directory and configure the project**:
   ```shell
   mkdir build
   cd build
   cmake ..
   ```


#### Compile and Configure Examples
To build a specific case study:
```shell
cmake --build . --target robot
```

#### Run Examples
To execute a specific case study:
```shell
./robot
```





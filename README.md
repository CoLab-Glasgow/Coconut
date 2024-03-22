# Coconut tool 
This Typestate Library is a tool that enables C++ users to define a protocol for their classes. This tool will significantly facilitate and secure the user’s work by detecting any violations of the protocol that the user specifies for objects.


### Download C++ ###
- On Windows, you can download it from [here.](https://docs.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=msvc-170/ "Install C and C++ support in Visual Studio") 

- On Mac, you can download the Xcode program from the Apple store, use Command Line Tool, and choose C++ in the Language section.

- On Linux, you can download it from [here.](https://learn.microsoft.com/en-us/cpp/linux/download-install-and-setup-the-linux-development-workload?view=msvc-170) 

  
### Download Cocount ###

```
git clone https://github.com/CoLab-Glasgow/Coconut.git

```
### Dependencies ###

- Boost library Version 1.81.0
``` 
git clone --branch boost-1.81.0 --depth 1 https://github.com/boostorg/boost.git

```

### Build the Coconut Library ##
- Navigate to the "Coconut" directory using the command:

```
cd Coconut

```
- Once inside the "Coconut" directory, run the following commands to build the library:
  

```
mkdir build
cd build 
cmake ... 
```
### Environment of the build ##
Windows 11 Pro, using Visual Studio Code as the IDE (Version: 1.87.2), with GCC version 13.2.0 (MinGW-W64 x86_64-msvcrt-posix-she)
### To Compile and configure examples with cmake ###

 ``` 
cmake --build . --target Client1

```
 



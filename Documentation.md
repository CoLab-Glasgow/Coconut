# Coconut Artefact

This artefact introduces the Coconut tool, which is aligned with discussions in our paper 
and offers typestate templates for the precise definition of objects and classes' typestate. 
It ensures adherence to protocols and is capable of detecting violations at compile-time. 
Specifically, it showcases its utility with examples from the embedded system field, such as the Robot example (discussed in the paper),
It supports branching, recursion, aliasing, concurrency, inheritance, and typestate visualisation. 
Moreover, this artefact includes a comparative evaluation, benchmarking Coconut's performance against three C++ implemented case studies across different metrics.


------------------------------------------------------------------------------------------------------------------------------------------

## Running the Coconut Tool

### Option 1: **Preferred** - Using Docker

#### ✅ Valid Example
1. **Install Docker**: Follow the instructions at [Docker's official documentation](https://docs.docker.com/get-docker/).
2. **Download Coconut file from here** (https://doi.org/10.5281/zenodo.14478714)
3. **Download the Docker Image**:
   - Download `coconut-project-v2.0.1.tar` from the repository.
   - Ensure Docker is running.
   - Load the Docker image:
     ```shell
     docker load --input coconut-project-v2.0.1.tar

     ```
     **Expected output:**
      ```
      Loaded image: coconut-project-v2.0.1:latest
     ```
4. **Start a Docker Container**:
   ```shell
   docker run -it coconut-project:v2.0.1
   ```
5. **Navigate to the Build Directory**:
   ```shell
   cd build
   ```
6. **Configure the Project with CMake**:
   ```shell
   cmake ..
   ```
7. **Compile and Build the Target**: for Robot example in Listings 1,2,3 in the paper
   ```shell
   cmake --build . --target robot
   ```
   **Expected output:**
   ```
   [100%] Built target robot
   ```
8. **Run the Executable**:
   ```shell
   ./robot
   ```
   **Expected output:**
   ```
   Start navigation...
   Arrived at destination. Picking up object...
   Object loaded. Moving to unload...
   Task complete. Returning to Idle...
   ```
9. **Compile and Build the Target**: for Robot example in Listings 4,5 in the paper

```shell
   cmake --build . --target robotOP
   ```
9. **Run the Executable**:

```shell
   ./robotOP
   ```
   **Expected output:**
   ```
   Start navigation...
   Arrived at destination. Picking up object...
   Object loaded. Moving to unload...
   Task complete. Returning to Idle...
   ```


#### ❌ Invalid Example (Optional)
To test invalid examples by breaking typestate specifications:
1. **Navigate to the Project Directory**:
   ```shell
   cd Robot
   ```
2. **Edit the Main File**: Uncomment line 5 using a text editor like `nano`:
   ```shell
   nano main.cpp
   ```
3. **Return to the Build Directory**:
   ```shell
   cd ..
   cd build
   ```
4. **Repeat Steps 5 and 6 from Valid Example**:
   **Expected output:**
   ```
   error : TYPESTATE No valid transition found for method ReleaseObject in this state
   gmake: *** [Makefile:176: robot] Error
   ```

---
#### (Optional) There are different examples available, along with other Robot examples. Try them by opening the CMakeLists file, locating the corresponding client, and experimenting with them the way with the Robot example.

## After running the examples, enter "exit" to terminate the Docker image.


### Option 2: Running Without Docker

#### Install C++
- **Windows**: [Install Visual Studio](https://docs.microsoft.com/en-us/cpp/build/vscpp-step-0-installation?view=msvc-170).
- **Mac**: Install Xcode from the Apple Store or use the Command Line Tool.
- **Linux**: [Set up C++ tools](https://learn.microsoft.com/en-us/cpp/linux/download-install-and-setup-the-linux-development-workload?view=msvc-170).

#### Clone the Repository
```shell
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

---

## 🖥️ Experiment-Study Virtual Machine
This virtual machine is designed for a comparative study of the **Coconut tool**, which employs typestate analysis, against other applications in embedded systems.
 The goal is to evaluate Coconut's performance and complexity reduction capabilities.

The `Coconut_Experiment.ova` file is a virtual machine image. Follow these steps to open and use it:

### Prerequisites
1. **Install VirtualBox**:
   - Download and install VirtualBox from [VirtualBox's official website](https://www.virtualbox.org/).


### Steps to Open the `.ova` File
1. **Import the `.ova` File**:
   - Open VirtualBox.
   - Go to `File > Import Appliance`.
   - Select `Coconut_Experiment.ova` and click `Next`.
   - Review the configuration and click `Import`.
2. **Start the Virtual Machine**:
   - Once the import is complete, select the virtual machine in the list.
   - Click `Start` to boot the virtual machine.
   - Then enter the password which is: Bash123.
3. **Access the Environment**:
   - The virtual machine contains folder which all necessary configurations and tools to replicate the experiment.

### 📁 Folder Structure

#### 1. `Performance-Analysis`

- **`Case_Studies`**: Original implementations of LightSwitch, PillBox, and Http-Connection with required libraries.
- **`Coconut-Examples`**: Coconut implementations of the same case studies.
- **`Compile-Time-Scripts`**: Scripts to measure compile times for original and Coconut implementations.
- **`RunTime_Scripts`**: Scripts to evaluate runtime performance.
- **`CPU_Scripts`**: Scripts to monitor CPU utilisation.
- **`Memory_Usage_Scripts`**: Scripts to monitor memory usage (Resident Set Size).
- **`CSV_Files`**: Contains per-run results and calculated averages.
- **`run_all.sh`**: Script to run all metrics, generate results, and save them in CSV files.


#### 2. `Complexity-Analysis`
- Scripts for evaluating code complexity for both Coconut and non-Coconut implementations.
- A `run_all.sh` script for batch execution of code complexity scripts.


### To run Performance metrics and generate table 2 in the paper (20-30 minutes) : Note that the results may vary slightly from those in the paper. However, they consistently show higher compile times for Coconut examples compared to the original examples. Additionally, Coconut examples consistently exhibit lower runtime, memory usage, and CPU consumption than the original examples.

Navigate to `Performance-Analysis` folder and open a terminal and enter this command:
```shell
./run_all.sh
```

### (optional) determine the number of iterations:
The command above will generate a table based on 30 iterations for compile-time, 100 run-time and 100 iterations for memory-usage and 30
for CPU.
However, it is possible to specify the number of script runs:
+ Step 1: Navigate to the `Performance-Analysis` folder. 
+ Step 2: Navigate to the `Compile-Time-Scripts` directory or any directory you prefer to see specific metrics
+ Step 3: run a specific script for example:
```shell
./measure_compile_time_pillbox_Coconut.sh 10
```




### To run code complexity and generate table 3 in the paper:
Navigate to `Complexity-Analysis` folder and open a terminal and enter this command:
```shell
./run_all.sh
```





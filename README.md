## IPC-with-shared-memory-and-semaphores

This project is a multi-process interprocess communication (IPC) system built using **POSIX shared memory** and **semaphores**. It enables the parent process to coordinate and exchange data with dynamically spawned child processes based on configuration instructions.

📘 Developed as part of the **Operating Systems** course at DIT UOA, this project demonstrates key systems programming concepts such as process management, synchronization, shared memory usage, and coordination of concurrent tasks.

---

## 🔧 How It Works

- A configuration file defines instructions such as spawning/terminating child processes or ending the parent.
- Each instruction includes:
  - The **target process ID**
  - A number of **repetitions**
  - A command: **`S`** (spawn), **`T`** (terminate), or **`E`** (exit parent)
- The parent reads this configuration via `ReadConfig()` and acts accordingly.

---

## 📌 Key Features

- **Dynamic Process Control**: Spawns or terminates child processes based on a config file.
- **Shared Memory Messaging**: Sends random lines from an input text file to random active children.
- **POSIX Semaphores**:
  - One per child for targeted wake-up.
  - A shared semaphore to block the parent while the child processes the line.
- **Safe File Logging**:
  - Each process logs received lines.
  - Synchronization ensures correct coordination, even if write order is not guaranteed.
- **Offset Optimization**:
  - Pre-computes line offsets in the input file.
  - Uses `fseek` to jump directly to random lines.

---

## 📁 File Structure

```
.
├── Parent.c         # Main process logic (configuration reading, process management)
├── Child.c          # Child process logic (receives and logs lines)
├── Makefile         # Build & run automation
├── config.txt       # Example configuration file
├── input.txt        # Source text file for line selection
└── README.md
```

---

## 🛠️ Build & Run

Use the included `Makefile` to compile and execute the program.

### 🔨 Compile

```bash
make
```

### 🚀 Run

```bash
make run
```

If you want to run processes separately (e.g., in two terminals):

```bash
make run-parent
make run-child
```

### 🧹 Clean

```bash
make clean
```

---

## 🧠 Logic Summary

- **Semaphores** ensure mutual exclusion and orderly execution between the parent and each child.
- Children terminate based on signals sent through shared memory flags.
- Each child logs:
  - The total number of messages received
  - The number of execution cycles it was active
- The parent gracefully shuts down all resources and processes when complete.

---

## ⚙️ Configuration Detail

- Config files use process numbers starting from 0 or 1.
- A parameter `adjust` allows adapting to different starting indices.

```c
int adjust = 1; // or 0 based on config style
```

---

## ✍️ Notes

- File write order from children may appear unordered due to asynchronous execution.
- Each child prefixes its messages with its process ID, ensuring clarity.
- If an action is redundant (e.g., terminating an already terminated process), it is safely ignored for simplicity.

---

# The-Debtors

WVU CS450 "The Debtors" 

CONTRIBUTORS:
- Hunter Lavender 
- Colby Gallaher
- Rae McDonald
- Garrett Rhodes

**MPX PREREQUISITES:**

Since Windows is not optimized for native MPX development. Instead, development shoud occurr in a Linux distribution or using Windows Subsystem for Linux.

Windows WSL:

WSL is an optional component of Windows 10 and later. First, ensure that WSL itself is enabled, and that a distribution is installed. Open a Command Prompt and run:

- wsl --install -d ubuntu

This will enable WSL if it has not already been done. The first time running a Linux session, a username and password will need to be created. This password will be the run needed to run "sudo" commands.

Ubuntu is the primary environment for MPX development. Once inside a Ubuntu session, open a Terminal and run:

- sudo apt update
- sudo apt install -y clang make nasm git binutils-i686-linux-gnu qemu-system-x86 gdb


macOS:

macOS comes with the clang C compiler natively, which is sufficient for MPX development. To install GNU make and other XCode tools needed for development, run:

- xcode-select --install

Next, install Homebrew. Homebrew is a package manager for macOS and Linux, which is useful for installing packages related to QEMU virtual memory. Run:

- -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

After running, follow the **Next Steps** prompt in the terminal window.

Finally, install NASM, QEMU, and the necessary cross-linker by running:

- brew install nasm qemu i686-elf-binutils i386-elf-gdb


**TO COMPILE AND EXECUTE:**

- Make sure all edited .c files are saved.
- Open terminal and "cd" into root directory.
- Run "make clean" in the root directory to rid all outdated dependency files (if compiling for first time, ignore).
- Run "make" to create a new Makefile and generate new dependencies.
- Run "/.mpx.sh" to execute and begin QEMU processes. This will boot user into OS.

**NOTE:** Only commit and push the modified/added .c files. Do not commit or push .o files created after running "make".

Interacting with the operating system is done via a command-line based "UNIX style" user interface. Once booted into the OS, run "help" to generate a list of available commands.

Working version R1 completed as of 09/05/23








# OS--mini-linux-shell
* This is program which mimics a shell inside a linux shell.
* The code uses fork() and execvp() commands to spawn a child process which run the command entered.
* The program also handles back-ground processes.
  * To run a command in back-ground add an **&** at the end of the command
  * e.g. **sleep 10 &** will run in background by the time you can run any other command
* User can enter multiple commands in a single line
  * If the commands are **&&** separated they will be executed in **series**
  * If the commands are **&&&** separated they will be executed in **parallel**
  * **Note:** at a given time the commands can be executed in series or in parallel and not in both
* **Batch mode**: User can pass a file as command line argument and the all the commands in file will run in serial manner
* The program also handles _ctrl + c_ signal
  * On _ctrl + c_ the shell will stop all the commands running in **fore-ground** and not ones running in the **back-ground**
* The user should run **exit** command to exit the shell program _ctrl + c_ will not work.
* Runnig the shell, to run the shell program type the follwing in your linux shell
  1. **make shell**
  1. **./shell [opt name of file for batch execution]**

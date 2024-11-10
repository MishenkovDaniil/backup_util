# Create and restore backup util

## How to use

- clone repository 
#### Backup util
- go to src directory and run in your terminal ```make backup```;
- run ```./my_backup <full/incremental> <work path> <backup path>```.
#### Restore util
- go to src directory and run in your terminal ```make restore```;
- run ```./my_restore <backup path> <work path>```.

Where:
- ```full/incremental``` $\text{--}$ backup modes: ```full``` means a full ```work``` backup, ```incremental``` - backup for modified files and directories only (compared to latest ```full``` backup)
- ```work path``` - path to a directory or file to backup 
- ```backup path``` - path to a backup storage directory

Notice that after backup there will be created a subdirectory with current date format name (```YYYY-MM-DD_HH-MM-SS```) where backup is stored.

Restore modifies changed files or create them if deleted but doesn't remove new files.

If u run ```backup``` for one file only (```work path``` is not a directory) and run 
```restore``` with this file removed ```work path``` mustn't contain any third party names (for example, ```work path ='./'``` or ```'../```).


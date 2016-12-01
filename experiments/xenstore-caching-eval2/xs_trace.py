from subprocess import call
from common import replace_confile
import os
import time

config_file_name = 'config.xl'
create_cmd = 'xl client create ' + config_file_name
create_eval_file = 'create_time.txt'
log_file = '/var/log/xs_log'
dest_file_prefix = 'xs_log_'
domname_prefix = 'mini_os_'
max_vms = 129
max_runs = 1
to_check = []

for i in range(0, 10):
    to_check.append(2**i)

print to_check
with open(config_file_name, 'r') as confile:
    config = confile.read()


for run in range(0, max_runs):
    for i in range(0, max_vms+1):
        if (os.path.isfile(create_eval_file)):
            os.remove(create_eval_file)

        if (os.path.isfile(log_file)):
            os.remove(log_file)

        replace_confile(config, i+1)
        
        call(create_cmd.split())
              
        while(not(os.path.isfile(create_eval_file))):
            time.sleep(1)


        if i in to_check:
            move_cmd = 'mv ' + log_file + ' ' + dest_file_prefix + str(run) + '_' + str(i)
            call(move_cmd.split())

        

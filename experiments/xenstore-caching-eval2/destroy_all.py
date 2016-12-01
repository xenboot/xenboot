import subprocess
import time

list_op_file = 'temp_xl_list_output'
list_cmd = 'xl list'
max_vms = 10

list_op = subprocess.Popen(list_cmd.split(), stdout=subprocess.PIPE).communicate()[0]
lines = list_op.split('\n')

to_destroy = []
    
for line in lines[2:len(lines)-1]:     # Exclude column names, Domain 0 and trailing blank line
    atts = line.split()
    if len(atts) == 6:
        domid = atts[1]
    else:
        domid = atts[0]

    to_destroy.append(domid) # = to_destroy + domid + ' '


#Not the most elegant solution, I know.
start = 0
while (len(to_destroy[start:]) > max_vms):
    destroy_now = to_destroy[start : start + max_vms]
    dest_cmd = 'xl client destroy ' + ' '.join(destroy_now)
    print(dest_cmd)
    subprocess.call(dest_cmd.split())
    time.sleep(5)
    start = start + max_vms

dest_cmd = 'xl client destroy ' + ' '.join(to_destroy[start:])
print (dest_cmd)

subprocess.call(dest_cmd.split())

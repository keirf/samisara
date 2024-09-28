# flash.py <hex_filename>

import os, sys, time, telnetlib

if sys.argv[1] == 'write':
    cmd = 'reset init ; flash write_image erase %s ; reset\n' % sys.argv[2]
elif sys.argv[1] == 'erase':
    cmd = 'reset init ; at32f4xx mass_erase ; reset\n'
else:
    print('Choose write or erase.')
    sys.exit(1)

# Start the OpenOCD daemon in the background and connect via telnet
def open_ocd():
    os.system('~/install/bin/openocd -s ~/install/share/openocd/scripts/ -f interface/cmsis-dap.cfg -f target/at32f4xx.cfg &')
    while True:
        time.sleep(0.5)
        try:
            t = telnetlib.Telnet('localhost', 4444)
        except:
            pass
        else:
            return t

with open_ocd() as t:
    t.write(cmd.encode('utf-8'))
    t.write('shutdown\n'.encode('utf-8'))
    t.read_all() # Waits for EOF (telnet session shutdown)

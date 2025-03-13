try:
    # The hidapi package exposes one or two top level modules: hid and,
    # optionally, hidraw. When both are available we prefer hidraw.
    import hidraw as hid
except ModuleNotFoundError:
    import hid

import struct, subprocess, sys

## Command set
class Cmd:
    Subreport       =  0
    DFU             =  1
    str = {
        Subreport: "Subreport",
        DFU: "DFU"
    }

## Command responses/acknowledgements
class Ack:
    Okay            =  0
    BadCommand      =  1

## Subreport indexes
class Subreport:
    Info            = 0
    BuildVer        = 1
    BuildDate       = 2

report_id = 0x01
report_length = 48

class Unit:
    def __init__(self, hid):
        self.hid = hid

    def _send_cmd(self, cmd, pkt):
        x = (struct.pack('3B', report_id, cmd, len(pkt) + 2)
             + pkt + bytes(report_length - len(pkt) - 2))
        self.hid.send_feature_report(x)

    def set_subreport(self, idx):
        self._send_cmd(Cmd.Subreport, struct.pack('<H', idx))

    def enter_dfu(self):
        self._send_cmd(Cmd.DFU, struct.pack('<I', 0xdeadbeef))

    def get_subreport(self, idx):
        self.set_subreport(idx)
        x = self.hid.get_feature_report(report_id, report_length+1)
        assert x[0] == report_id
        assert x[1] == idx
        return bytes(x[3:3+x[2]])

    def build_ver(self):
        x = self.get_subreport(Subreport.BuildVer)
        return x.decode('utf-8')

    def build_date(self):
        x = self.get_subreport(Subreport.BuildDate)
        return x.decode('utf-8')

def print_info_line(name: str, value: str, tab=0) -> None:
    print(''.ljust(tab) + (name + ':').ljust(12-tab) + value)

def find_samisara():
    devs = hid.enumerate()
    r = None
    for d in devs:
#        print(d)
        if 'Samisara' in d['product_string'] and d['usage_page'] == 0xffc1:
            r = d
    return r

# Check that dfu-util and <dfu_file> both exist before resetting Samisara
def check_dfu_util(dfu_file):
    dfu_cmd = ['dfu-util', '-l']
    subprocess.run(dfu_cmd, capture_output=True)
    open(dfu_file, 'rb')

# Run dfu-util to re-flash Samisara
def run_dfu_util(dfu_file):
    dfu_cmd = ['dfu-util', '-w', '-d', '2e3c:df11', '-a', '0',
               '-D', dfu_file, '-s', ':leave']
    subprocess.run(dfu_cmd)

def usage():
    print('Usage: samisara <cmd> <args...>', file=sys.stderr)
    print('Commands:', file=sys.stderr)
    print('  info', file=sys.stderr)
    print('  dfu <dfu_file>', file=sys.stderr)
    sys.exit(1)

def main(argv):

    if len(argv) < 2:
        usage()

    d = find_samisara()
    h = hid.device()
    if d is not None:
        h.open_path(d['path'])
    elif argv[1] != 'dfu':
        print('Device not found', file=sys.stderr)
        sys.exit(1)

    sami = Unit(h)

    cmd = argv[1]
    argv = argv[2:]

    if cmd == 'info':
        if len(argv) != 0:
            usage()
        print('Device:')
        print_info_line('Port', d['path'].decode('utf-8', 'ignore'), tab=2)
        print_info_line('Firmware', f'{sami.build_ver()}'
                        f' ({sami.build_date()})', tab=2)
        serial = h.get_serial_number_string()
        print_info_line('Serial', serial if serial else 'Unknown', tab=2)
    elif cmd == 'dfu':
        if len(argv) != 1:
            usage()
        dfu_file = argv[0]
        check_dfu_util(dfu_file)
        if d is not None:
            sami.enter_dfu()
        run_dfu_util(dfu_file)
    else:
        usage()

if __name__ == "__main__":
    main(sys.argv)

# Local variables:
# python-indent: 4
# End:

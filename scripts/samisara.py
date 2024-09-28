import hid

report_id = 0x01
report_length = 48

def find_samisara():
    devs = hid.enumerate()
    r = None
    for d in devs:
        print(d)
        if 'Samisara' in d['product_string'] and d['usage_page'] == 0xffc1:
            r = d
    return r

d = find_samisara()

h = hid.device()
h.open_path(d['path'])
print(h)
out = bytes([report_id])
for i in range(report_length):
    out += bytes([i+10])
h.send_feature_report(out)
x = h.get_feature_report(report_id, report_length+1)
print(len(x), x)

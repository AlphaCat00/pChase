import subprocess
import sys

CACHELINES = [1, 2, 3, 4, 6, 8, 12, 16, 24, 32]

P_CHASE = sys.argv[1]
MEM_BIND = sys.argv[2]
MEM_BIND_1 = sys.argv[3]
output = sys.argv[4]

f_out = open(f"reformat-{output}", "w")

for access in (("load", "load"),("store", "load"),("load", "store"),("store", "store")):
    for m_bind in ((0, 0), (0, MEM_BIND), (MEM_BIND, 0), (MEM_BIND, MEM_BIND), (MEM_BIND, MEM_BIND_1)):
        # f_out = open(f"{'_'.join(access)}-{m_bind[0]}_{m_bind[1]}-{output}", "w")
        f_out.write(f",core 0(victim), core 1\n")
        f_out.write(f"access,{','.join(access)}\n")
        f_out.write(f"numa node,{m_bind[0]},{m_bind[1]}\n")
        f_out.write('cacheline number,memory latency (ns),memory bandwidth (MB/s)\n')
        buf = []
        for blk_size in CACHELINES:
            conf = ['2', f"0 -c 1m -p 1m -m {access[0]} 1 -a random -n map 0:{m_bind[0]} -s 3.0 -o csv",
                    f"1 -c 2m -p 2m -m {access[1]} {blk_size} -a random -n map 0:{m_bind[1]} -s 3.0 -o csv"]
            conf_str = "\n".join(conf)
            with open("pchase.conf", "w") as f:
                f.write(conf_str)
            with subprocess.Popen([P_CHASE], stdout=subprocess.PIPE, encoding="utf-8") as proc:
                out_str = proc.stdout.read()
                buf.append(out_str.split(','))
                print(out_str)
            # print(conf_str)
        # f_out.close()
        for line in zip(map(str, CACHELINES), *list(zip(*buf))[-2:]):
            f_out.write(','.join(line))
        f_out.write('\n\n')
f_out.close()

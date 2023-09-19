import subprocess
import sys

CACHELINES = [1, 2, 3, 4, 6, 8, 12, 16, 24, 32]

P_CHASE = sys.argv[1]
MEM_BIND = sys.argv[2]
MEM_BIND_1 = sys.argv[3]
output = sys.argv[4]

for access in (("load", "load"),("store", "load"),("load", "store"),("store", "store")):
    for m_bind in ((0, 0), (0, MEM_BIND), (MEM_BIND, 0), (MEM_BIND, MEM_BIND), (MEM_BIND, MEM_BIND_1)):
        f_out = open(f"{'_'.join(access)}-{m_bind[0]}_{m_bind[1]}-{output}", "w")
        for blk_size in (64*x for x in CACHELINES):
            conf = ['2', f"0 -c 1m -p 1m -m {access[0]} 1 -a random -n map 0:{m_bind[0]} -s 3.0 -o csv",
                    f"1 -c 1m -p 1m -m {access[1]} {blk_size} -a random -n map 0:{m_bind[1]} -s 3.0 -o csv"]
            conf_str = "\n".join(conf)
            with open("pchase.conf", "w") as f:
                f.write(conf_str)
            with subprocess.Popen([P_CHASE], stdout=subprocess.PIPE, encoding="utf-8") as proc:
                out_str = proc.stdout.read()
                f_out.write(out_str)
                print(out_str)
            # print(conf_str)
        f_out.close()

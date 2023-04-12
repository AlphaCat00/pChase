import subprocess
import sys

P_CHASE = sys.argv[1]
MEM_BIND = sys.argv[2]
output = sys.argv[3]

for access in (("load", "load"), ("load", "store"), ("store", "store"), ("store", "load"),):
    for thread in (("intra", 2), ("inter", 2), ("inter", 4)):
        f_out = open(f"{'_'.join(access)}-{thread[0]}_{thread[1]}-{output}", "w")
        with subprocess.Popen([P_CHASE, "-o", "hdr"], stdout=subprocess.PIPE, encoding="utf-8") as proc:
            out_str = proc.stdout.read()
            f_out.write(out_str)
            print(out_str)

        for blk_size in map(lambda x: 1 << x, range(0, 7)):
            conf = [str(thread[1]), f"0 -c 32m -p 32m -m {access[0]} 1 -a random -n map 0:{MEM_BIND} -s 3.0 -o csv"]
            for i in range(1, thread[1]):
                core_id = 0 if thread[0] == "intra" else i
                conf.append(
                    f"{core_id} -c 32m -p 32m -m {access[1]} {blk_size} -a random -n map 0:{MEM_BIND} -s 3.0 -o csv")

            conf_str = "\n".join(conf)
            with open("pchase.conf", "w") as f:
                f.write(conf_str)
            with subprocess.Popen([P_CHASE], stdout=subprocess.PIPE, encoding="utf-8") as proc:
                out_str = proc.stdout.read()
                f_out.write(out_str)
                print(out_str)
            # print(conf_str)
        f_out.close()

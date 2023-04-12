import subprocess
import sys

P_CHASE = sys.argv[1]
MEM_BIND = sys.argv[2]
output = sys.argv[3]
for thread in (("inter", 2),):
    for access in (("load", "load"), ("load", "store"), ("store", "store"), ("store", "load"),):
        f_out = open(f"{'_'.join(access)}-{thread[0]}_{thread[1]}-{output}", "w")
        for blk_size in ((64, 64), (64, 1), (1, 64)):
            conf = [str(thread[1]),
                    f"0 -c 32m -p 32m -m {access[0]} {blk_size[0]} -a random -n map 0:{MEM_BIND} -s 3.0 -o csv"]
            core_id = 0 if thread[0] == "intra" else 1
            conf.append(
                f"{core_id} -c 32m -p 32m -m {access[1]} {blk_size[1]} -a random -n map 0:{MEM_BIND} -s 3.0 -o csv")

            conf_str = "\n".join(conf)
            with open("pchase.conf", "w") as f:
                f.write(conf_str)
            with subprocess.Popen([P_CHASE], stdout=subprocess.PIPE, encoding="utf-8") as proc:
                out_str = proc.stdout.read()
                f_out.write(out_str)
                print(out_str)
            # print(conf_str)
        f_out.close()

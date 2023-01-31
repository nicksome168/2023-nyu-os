import sys
import subprocess
import unittest


class Test(unittest.TestCase):
    fname = ""

    def test(self):
        with open(self.fname, "r") as f:
            lines = f.readlines()
            for line in lines:
                args = line.split()
                exp = "[./nyuc] -> [./NYUC] [./nyuc]"
                for arg in args:
                    exp += "\n[{}] -> [{}] [{}]".format(
                        arg, arg.upper(), arg.lower())
                cmd = "./nyuc "
                cmd += " ".join(args)
                res = subprocess.run(
                    cmd.split(), stdout=subprocess.PIPE, encoding='UTF-8')
                self.assertEqual(res.stdout, exp+"\n")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: {} <filename>".format(sys.argv[0]))
        sys.exit(1)
    Test.fname = sys.argv[1]
    unittest.main(argv=['first-arg-is-ignored'], exit=False)

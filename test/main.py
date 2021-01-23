import colorama
import filecmp
import os
import prettytable
import shutil
import subprocess
import sys

working_dir = "test/.tmp"
tests = []
output = None
failed = 0
def Test(fn):
	tests.append(fn)

def encode(input_file, simple):
	base = os.path.basename(input_file)
	output = os.path.join(working_dir, base + (".ch" if simple else ".cmh"))
	p = subprocess.Popen([
		"bin/markovhuffman.exe",
		input_file,
		"-o", output,
		"-h" if simple else "-",
		"-d", os.path.join(working_dir, base + (".eh" if simple else ".e"))
	], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	out, err = p.communicate()
	if p.returncode != 0:
		print("Error while encoding")
		print(err)
		sys.exit(1)
	return output

def decode(input_file, simple):
	base = os.path.basename(input_file)
	output = os.path.join(working_dir, base + (".dh" if simple else ".dmh"))
	p = subprocess.Popen([
		"bin/markovhuffman.exe",
		os.path.join(working_dir, base + (".ch" if simple else ".cmh")),
		"-o", output,
		"-xh" if simple else "-x",
		"-e", os.path.join(working_dir, base + (".eh" if simple else ".e"))
	], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	out, err = p.communicate()
	if p.returncode != 0:
		print("Error while decoding")
		print(err)
		sys.exit(1)
	return output

def gzbz(input_file, gz):
	base = os.path.basename(input_file)
	output = os.path.join(working_dir, base + (".gz" if gz else ".bz2"))
	f = open(output, "wb")
	p = subprocess.Popen([
		"gzip" if gz else "bzip2",
		"-ck9" if gz else "-ckz9",
		input_file
	], stdout=f)
	out, err = p.communicate()
	if p.returncode != 0:
		print("Error while compressing using gzip or bzip2")
		print(err)
		sys.exit(1)
	return output

def run_test(input_file):
	assert(os.path.exists(input_file))
	print("checking {}...".format(input_file))
	e1 = encode(input_file, True)
	e2 = encode(input_file, False)
	o1 = decode(input_file, True)
	o2 = decode(input_file, False)
	c1 = gzbz(input_file, True)
	c2 = gzbz(input_file, False)
	correct = filecmp.cmp(input_file, o1) and filecmp.cmp(input_file, o2)
	input_size = os.path.getsize(input_file)
	x1 = os.path.getsize(e1) / input_size
	x2 = os.path.getsize(e2) / input_size
	output.add_row([
		os.path.basename(input_file),
		colorama.Style.BRIGHT + (colorama.Fore.GREEN + "Good" if correct else colorama.Fore.RED + "FAILED") + colorama.Style.RESET_ALL,
		"{:.02f}".format(x1),
		#"{:.02f} ({:.0f}%)".format(x2, 100 * ((x2 - x1) / x1)),
		"{:.02f} ({:.0f}%)".format(x2, 100 * (x2 - x1)), # Yes I know, adding percentages is bad. Makes some sense here, though.
		"{:.02f}".format(os.path.getsize(c1) / input_size),
		"{:.02f}".format(os.path.getsize(c2) / input_size)
	])
	if not correct:
		global failed
		failed += 1

@Test
def test_a():
	run_test("test/input/input_a.txt")

@Test
def test_b():
	run_test("test/input/input_b.txt")

@Test
def test_ipsum():
	run_test("test/input/input_ipsum.txt")

@Test
def test_wiki_cpp():
	run_test("test/input/input_wiki_cpp.txt")

@Test
def test_wiki_cpp_html():
	run_test("test/input/input_wiki_cpp.html")

def main():
	if os.path.exists(working_dir):
		print("Error: .tmp path exists.")
		sys.exit(1)
	
	print("compiling...")
	p = subprocess.Popen(["make"], stderr=subprocess.PIPE)
	out, err = p.communicate()
	if p.returncode != 0:
		print("make failed:")
		print(err)
		sys.exit(1)

	print("running...")
	global output
	output = prettytable.PrettyTable()
	output.field_names = ["Test file", "Status", "Huffman", "Markov-Huffman", "gz", "bz"]
	output.align = "l"
	# setup workspace
	os.mkdir(working_dir)
	# run tests
	[_() for _ in tests]
	# cleanup
	shutil.rmtree(working_dir)
	print(output)
	if failed:
		sys.exit(1)

main()

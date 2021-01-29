import colorama
import filecmp
import os
import prettytable
import shutil
import subprocess
import sys

working_dir = "test/.tmp"
exe = "bin/markovhuffman.exe" if sys.platform == "win32" else "bin/markovhuffman"
tests = []
output = None
failed = 0
def Test(fn):
	tests.append(fn)

def encode(input_file, simple):
	base = os.path.basename(input_file)
	output = os.path.join(working_dir, base + (".ch" if simple else ".cm"))
	table = os.path.join(working_dir, base + (".eh" if simple else ".e"))
	p = subprocess.Popen([
		exe,
		input_file,
		"-o", output,
		"-h" if simple else "-",
		"-d", table
	], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	out, err = p.communicate()
	if p.returncode != 0:
		print("Error while encoding")
		print(err.decode("utf-8"))
		sys.exit(1)
	return (output, table)

def decode(input_file, encoded_file, encoding_table, simple):
	base = os.path.basename(input_file)
	output = os.path.join(working_dir, base + (".dh" if simple else ".dm"))
	p = subprocess.Popen([
		exe,
		encoded_file,
		"-o", output,
		"-xh" if simple else "-x",
		"-e", encoding_table
	], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
	out, err = p.communicate()
	if p.returncode != 0:
		print("Error while decoding")
		print(err.decode("utf-8"))
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
		print(err.decode("utf-8"))
		sys.exit(1)
	return output

def run_test(input_file):
	assert(os.path.exists(input_file))
	print("checking {}...".format(input_file))
	# run through encoders and decoders
	encoded_huffman, encoding_table_huffman = encode(input_file, True)
	encoded_markov,  encoding_table_markov  = encode(input_file, False)
	decoded_huffman = decode(input_file, encoded_huffman, encoding_table_huffman, True)
	decoded_markov  = decode(input_file, encoded_markov, encoding_table_markov, False)
	# check correctness
	correct = filecmp.cmp(input_file, decoded_huffman) and filecmp.cmp(input_file, decoded_markov)
	# find compression ratio
	input_size = os.path.getsize(input_file)
	x1 = os.path.getsize(encoded_huffman) / input_size
	x2 = os.path.getsize(encoded_markov) / input_size
	# test against gz and bz2 compression utilities
	compressed_gz  = gzbz(input_file, True)
	x3 = os.path.getsize(compressed_gz) / input_size
	compressed_bz2 = gzbz(input_file, False)
	x4 = os.path.getsize(compressed_bz2) / input_size
	# test combination
	encoded_gz_markov, _ = encode(compressed_gz, False)
	x5 = os.path.getsize(encoded_gz_markov) / input_size
	encoded_bz2_markov, _ = encode(compressed_bz2, False)
	x6 = os.path.getsize(encoded_bz2_markov) / input_size
	output.add_row([
		os.path.basename(input_file),
		colorama.Style.BRIGHT + (colorama.Fore.GREEN + "Good" if correct else colorama.Fore.RED + "FAILED") + colorama.Style.RESET_ALL,
		"{:.02f}".format(x1),
		"{:.02f} ({:.0f}%)".format(x2, 100 * ((x2 - x1) / x1)),
		#"{:.02f} ({:.0f}%)".format(x2, 100 * (x2 - x1)), # Yes I know, adding percentages is bad. Makes some sense here, though.
		"{:.02f}".format(os.path.getsize(compressed_gz) / input_size),
		"{:.02f}".format(x4),
		"{:.02f} ({:.0f}%)".format(x5, 100 * ((x5 - x3) / x3)),
		"{:.02f} ({:.0f}%)".format(x6, 100 * ((x6 - x4) / x4))
	])
	if not correct:
		global failed
		failed += 1

#@Test
#def test_a():
#	run_test("test/input/input_a.txt")
#
#@Test
#def test_b():
#	run_test("test/input/input_b.txt")

@Test
def test_ipsum():
	run_test("test/input/input_ipsum.txt")

@Test
def test_wiki_cpp():
	run_test("test/input/input_wiki_cpp.txt")

@Test
def test_wiki_cpp_html():
	run_test("test/input/input_wiki_cpp.html")

@Test
def test_exe():
	run_test(exe)

def main():
	if os.path.exists(working_dir):
		print("Error: .tmp path exists.")
		sys.exit(1)
	
	print("compiling...")
	p = subprocess.Popen(["make"], stderr=subprocess.PIPE)
	out, err = p.communicate()
	if p.returncode != 0:
		print("make failed:")
		print(err.decode("utf-8"))
		sys.exit(1)

	print("running...")
	global output
	output = prettytable.PrettyTable()
	output.field_names = ["Test file", "Status", "Huffman", "Markov-Huffman", "gz", "bz", "gz + Markov-Huffman", "bz + Markov-Huffman"]
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

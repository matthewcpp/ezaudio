import sys
import os
import re

def main():
	script_dir = os.path.dirname(os.path.realpath(__file__))
	header_path = os.path.join(script_dir, "..", "include", "ezaudio", "ezaudio.h")
	src_folder = os.path.join(script_dir, "..", "src")

	with open(sys.argv[1], 'w') as output_file:
		with open(header_path, 'r') as header_file:
			output_file.write(header_file.read())

		output_file.write("\n#ifdef EZAUDIO_IMPLEMENTATION\n")
		add_source_file(os.path.join(src_folder, "common.cpp"), None, output_file)
		add_source_file(os.path.join(src_folder, "wsapi.cpp"), "_WIN32", output_file)
		add_source_file(os.path.join(src_folder, "coreaudio.cpp"), "__APPLE__", output_file)
		output_file.write("\n#endif\n")
	return 0

def add_source_file(source_path, guard, output_file):
	if guard is not None:
		output_file.write("\n#ifdef {}\n".format(guard))

	separator = "\n// {}\n".format('*' * 10)
	output_file.write("{}// {}{}".format(separator, os.path.basename(source_path), separator))

	stripped_include = False
	with open(source_path, 'r') as source_file:
		for line in source_file.readlines():
			if not stripped_include and re.match('\s*#\s*include\s*"ezaudio/ezaudio.h"\s*', line):
				stripped_include = True
			else:
				output_file.write(line)

	if guard is not None:
		output_file.write("\n#endif\n")

if __name__ == "__main__":
	if len(sys.argv) < 2:
		print("Usage: python make_header_only.py /path/to/output.h")
		sys.exit(1)

	sys.exit(main())

import PyInstaller.__main__ as main
import os, sys

print("Building CCGui")

if len(sys.argv) != 2:
    print("Invalid number of arguments. Usage PyInstaller.py {distroPath}")
    sys.exit(1)

filePath = os.path.dirname(os.path.realpath(__file__))
filePath += ("/Main.py")

print(f"File path found: {filePath}")

distPath = os.path.realpath(sys.argv[1])
print(f"Distro path found: {distPath}")

main.run([
    '--name=CCGui',
    '--onefile',
    f"--distpath={distPath}",
    f"{filePath}"
])
import os
import sys

def main():
    is_writeable = os.access(sys.argv[1], os.W_OK)
    return_code = int(not is_writeable)
    sys.exit(return_code)

if __name__ == "__main__":
    main()

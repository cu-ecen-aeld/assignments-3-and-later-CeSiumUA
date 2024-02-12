if [ $# -lt 2 ]; then
    echo "Not enough arguments supplied"
    exit 1
fi

WRITEFILE=$1
WRITESTR=$2

mkdir -p "$(dirname "$WRITEFILE")"

echo "$WRITESTR" > "$WRITEFILE"

if [ ! $? -eq 0 ]; then
    echo "Could not create file $WRITEFILE"
    exit 1
fi
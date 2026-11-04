#!/bin/bash

set -euo pipefail

echo "Checking for Python installation..."
if command -v python3 >/dev/null 2>&1; then
    PYTHON_CMD=python3
elif command -v python >/dev/null 2>&1; then
    PYTHON_CMD=python
else
    echo "Error: Python does not appear to be installed. If you think this is not the case, you should check to make sure it was added to your system PATH at the time of installation."
    echo "Please ensure Python 3.12 or higher is installed on your system, and then run this installer again."
    exit 1
fi

# overseer_model uses f-string syntax introduced in Python 3.12 (PEP 701)
if ! "$PYTHON_CMD" -c 'import sys; sys.exit(0 if sys.version_info >= (3, 12) else 1)'; then
    echo "Error: found $("$PYTHON_CMD" --version 2>&1) ($(command -v "$PYTHON_CMD")), but Python 3.12 or higher is required."
    echo "Please install Python 3.12 or higher and run this installer again."
    exit 1
fi

echo "Checking for make..."
if ! command -v make >/dev/null 2>&1; then
    echo "Error: make does not appear to be installed or is not available on PATH."
    echo "Please install make and a C++ compiler, then run this installer again."
    exit 1
fi

echo "All checks passed. Proceeding to installation."

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Creating virtual environment..."
"$PYTHON_CMD" -m venv "$SCRIPT_DIR/venv"
source "$SCRIPT_DIR/venv/bin/activate"

echo "Installing Overseer..."
"$PYTHON_CMD" -m pip install -e "$SCRIPT_DIR/Overseer"
echo "Successfully installed Overseer!"
echo ""
echo "Building simulation binary..."
make
echo "Successfully created binary!"
echo ""
echo "Configuring files..."
USER_CONFIG_DIR="$(python -c '
from platformdirs import user_config_dir
print(user_config_dir("Overseer", False, roaming=True))
'
)"
USER_CONFIG_FILE="$USER_CONFIG_DIR/config.yml"
USER_MODELS_DIR="$SCRIPT_DIR"
USER_LOGS_DIR="$SCRIPT_DIR/logs"

mkdir -p "$USER_CONFIG_DIR"
mkdir -p "$USER_LOGS_DIR"

WRITE_CONFIG=true
if [[ -e "$USER_CONFIG_FILE" ]]; then
    echo
    echo "An Overseer configuration file already exists:"
    echo "   $USER_CONFIG_FILE"
    echo "If we replaced it, you wouldn't lose much, but just in case..."

    read -r -p "Replace it with the Labor-Time-Economy configuration? [y/N] " RESPONSE

    case "$RESPONSE" in
        [yY]|[yY][eE][sS])
            WRITE_CONFIG=true
            ;;
        *)
            echo "Skipping configuration step."
            WRITE_CONFIG=false
            ;;
    esac
fi

if [[ "$WRITE_CONFIG" == true ]]; then
    cat > "$USER_CONFIG_FILE" <<EOF
global_settings:
    default_save_dir: "$SCRIPT_DIR"
    user_models_dir: "$USER_MODELS_DIR"
    user_logs_dir: "$USER_LOGS_DIR"
    run_on_startup: true
EOF
fi

BC_DEMO_FILE="$SCRIPT_DIR/overseer_model/bc_lte_demo.yml" \
SIM_PATH="$SCRIPT_DIR/bin/sim" \
MODEL_PATH="$USER_MODELS_DIR/overseer_model" \
"$PYTHON_CMD" "$SCRIPT_DIR/overseer_model/prepare_demo_and_params.py"

LAUNCHER_FILE="$SCRIPT_DIR/run.sh"

cat > "$LAUNCHER_FILE" <<EOF
#!/bin/bash

set -euo pipefail

SCRIPT_DIR="\$(cd -- "\$(dirname -- "\${BASH_SOURCE[0]}")" && pwd)"
source "\$SCRIPT_DIR/venv/bin/activate"
cd "\$SCRIPT_DIR"

exec python -m overseer
EOF

chmod +x "$LAUNCHER_FILE"

echo
echo "Setup complete."
echo "To launch the application, run:"
echo
echo "    ./run.sh"
echo 
echo "Additional details which might be relevant to you:"
echo "If you want the rest of the models that come packaged with Overseer, " \
    "go into the settings and change your 'User models directory to a " \
    "directory where you want your user data to go (make sure that the " \
    "exact folder does not exist yet). The models will be created there " \
    "the next time that you launch Overseer. You can then move the overseer_model " \
    "directory here to that user models folder and have everything in one place."

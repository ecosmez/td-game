import json
import sys
import time

ENGINE_REMOTE = r"C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python"
sys.path.insert(0, ENGINE_REMOTE)
from remote_execution import RemoteExecution, MODE_EXEC_FILE

SCRIPT = r"C:\Users\Eduardo Cosme\Documents\Unreal Projects\td-game\Content\Python\_hardfix_createwidget.py"


def main():
    remote = RemoteExecution()
    remote.start()
    try:
        node_id = None
        for _ in range(30):
            nodes = remote.remote_nodes
            if nodes:
                node_id = nodes[0]["node_id"]
                break
            time.sleep(0.5)
        if not node_id:
            print("NO_REMOTE", remote.remote_nodes)
            return 2
        print("node", node_id)
        remote.open_command_connection(node_id)
        result = remote.run_command(SCRIPT, unattended=True, exec_mode=MODE_EXEC_FILE)
        print(json.dumps(result, indent=2, default=str))
        return 0 if result.get("success", True) else 1
    finally:
        remote.stop()


if __name__ == "__main__":
    raise SystemExit(main())

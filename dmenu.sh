#!/bin/bash
items=""

cd ~/.tmux_sessions
for filename in *; do
	if [ "$(echo $filename | grep '.conf')" != "" ]; then
		items+="${filename%.conf}\n"
	fi
done

item=$(echo -ne $items | dmenu)
eval xterm -e "tss run $item"

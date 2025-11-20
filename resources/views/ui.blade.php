<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8" />
    <title>Log Explorer</title>
    <style>
        body {
            font-family: monospace;
            margin: 0;
            background: #f5f5f5;
        }

        h2 {
            margin: 10px;
        }

        #container {
            display: flex;
            height: 90vh;
        }

        #tree {
            width: 250px;
            border-right: 1px solid #ccc;
            background: #fff;
            padding: 8px;
            overflow-y: auto;
        }

        #tree div {
            cursor: pointer;
            padding: 4px;
        }

        #tree div:hover {
            background: #eef;
        }

        #output {
            flex: 1;
            margin: 0;
            padding: 10px;
            overflow-y: scroll;
            background: #111;
            color: #0f0;
        }
    </style>
</head>

<body>
    <div id="files"></div>
    <h2>🗂 Log Explorer</h2>
    <div id="container">
        <div id="tree"></div>
        <pre id="output"></pre>
    </div>
    <script>
        let currentFile = null;
        let ws = new WebSocket(`ws://${window.location.hostname}:${Number(window.location.port) + 1}`);
        ws.onopen = () => {
            ws.send(JSON.stringify({
                action: "subscribe",
                file: currentFile
            }))
        }

        function switchFile(newFile) {
            ws.send(JSON.stringify({
                action: "unsubscribe",
                file: currentFile
            }));
            ws.send(JSON.stringify({
                action: "subscribe",
                file: newFile
            }));
            currentFile = newFile;
        }

        ws.onmessage = (event) => {
            const output = document.getElementById("output");
            output.textContent += event.data;
            window.scrollTo(0, document.body.scrollHeight);
        };

        async function loadFolder(folder = null) {
            const response = await fetch('/logwatcher/folder', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    folder
                })
            })
            const data = await response.json()
            let html = "";
            data.forEach(item => {
                if (item.is_dir) {
                    html += `<div onclick="loadFolder('${item.path}')">📁 ${item.name}</div>`;
                } else {
                    html += `<div onclick="openFile('${item.path}')">📄 ${item.name}</div>`;
                }
            });
            
            document.getElementById("tree").innerHTML = html;
        }
        window.onload = () => loadFolder()
    </script>
</body>

</html>
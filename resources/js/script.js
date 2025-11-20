const API_BASE = "http://localhost:8888";

async function loadFolder(path = "logs") {
  // const res = await fetch(`${API_BASE}/api/list?path=${path}`);
  // const data = await res.json();
  let data = {
    folders: [],
    files: ['app.log', 'new.log', 'uuu.log']
  };
  let html = "";
  data.folders.forEach(f => {
    html += `<div onclick="loadFolder('${path}/${f}')">📁 ${f}</div>`;
  });
  data.files.forEach(f => {
    html += `<div onclick="openFile('${path}/${f}')">📄 ${f}</div>`;
  });
  document.getElementById("tree").innerHTML = html;
}

// let socket;
function openFile(path) {
  let ws = new WebSocket("ws://localhost:8888?filepath=" + path);
  ws.onmessage = (event) => {
    // console.log(event.data, `${i}-client`)
    const output = document.getElementById("output");
    output.textContent += event.data;
    window.scrollTo(0, document.body.scrollHeight);
  };
  // for (let i = 1; i <= 300; i++) {
  //   // ws.onopen = () => console.log(`✅ Connected ${i}-client`);
  //   // ws.onclose = () => console.log("🔒 Connection closed");

  // }
  // if (socket) {
  // //   socket.close();
  // // }
  // socket = new WebSocket(API_BASE);

  // // 1. onopen (equivalent to 'onconnected')
  // // This event fires once the connection is established and the handshake is complete.
  // socket.onopen = (event) => {
  //   console.log("WebSocket connected!", event);
  //   // You can start sending messages now
  //   socket.send("Hello from the browser!");
  // };

  // // You can also use addEventListener for all these events:
  // // socket.addEventListener('open', (event) => { ... });


  // // 2. onmessage
  // // This event fires every time data is received from the server.
  // socket.onmessage = (event) => {
  //   console.log("Message from server: ", event.data);
  //   // event.data contains the actual message payload (string or binary data)
  // };


  // // 3. onclose
  // // This event fires when the connection is closed, either by the client, the server, or due to network issues.
  // socket.onclose = (event) => {
  //   console.log("WebSocket disconnected.", event);
  //   if (event.wasClean) {
  //     console.log(`Closed cleanly, code=${event.code}, reason=${event.reason}`);
  //   } else {
  //     // e.g., server process killed or network error
  //     console.log('Connection died unexpectedly');
  //   }
  // };

  // // 4. onerror (Important for robust handling)
  // // This event fires if an error occurs during communication. An error event is usually followed by a close event.
  // socket.onerror = (error) => {
  //   console.error("WebSocket error: ", error);
  // };

  // // Function to manually close the connection
  // function closeConnection() {
  //   if (socket.readyState === WebSocket.OPEN) {
  //     // The close() method can optionally take a code and a reason
  //     socket.close(1000, "User requested disconnection");
  //   }
  // }
}
loadFolder();
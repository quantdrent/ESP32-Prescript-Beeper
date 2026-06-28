#pragma once

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Prescript Console</title>
<style>
*{box-sizing:border-box}
body{
  margin:0;
  background:#000;
  color:#99edff;
  font-family:Arial,sans-serif;
}
.wrap{
  width:min(760px, 92vw);
  margin:0 auto;
  padding:28px 0 40px;
}
h1{
  margin:0 0 18px;
  font-size:28px;
  text-align:center;
  letter-spacing:.12em;
}
.row{
  display:flex;
  gap:10px;
  margin-bottom:10px;
}
input, button{
  width:100%;
  border:2px solid #99edff;
  border-radius:0;
  background:#000;
  color:#99edff;
  padding:12px 14px;
  font-size:15px;
}
input:focus{
  outline:none;
  box-shadow:none;
}
.textInput{
  flex:1;
}
.secondsWrap{
  width:92px;
  position:relative;
}
.secondsWrap input{
  padding-right:28px;
  text-align:center;
}
.secondsWrap span{
  position:absolute;
  right:12px;
  top:50%;
  transform:translateY(-50%);
  color:#99edff;
  pointer-events:none;
}
button{
  cursor:pointer;
  text-transform:uppercase;
}
.status{
  min-height:22px;
  margin:8px 0 14px;
}
table{
  width:100%;
  border-collapse:collapse;
  font-size:14px;
  table-layout:fixed;
}
th,td{
  padding:10px 8px;
  border:1px solid #99edff;
  text-align:left;
}
th{
  font-size:12px;
  text-transform:uppercase;
  letter-spacing:.12em;
}
.rowActions{
  display:flex;
  gap:8px;
  align-items:center;
}
.rowActions input{
  min-width:72px;
  max-width:72px;
  text-align:center;
}
@media (max-width:720px){
  .row{
    flex-direction:column;
  }
  .secondsWrap{
    width:100%;
  }
  table, thead, tbody, tr, th, td{display:block}
  thead{display:none}
  tr{
    margin-bottom:12px;
  }
  td{
    border-width:1px;
    padding:10px;
  }
  td::before{
    content:attr(data-label);
    display:block;
    font-size:11px;
    text-transform:uppercase;
    letter-spacing:.1em;
    margin-bottom:4px;
  }
  .rowActions{
    flex-direction:column;
    align-items:stretch;
  }
}
</style>
</head>
<body>
<div class="wrap">
  <h1>PRESCRIPT CONSOLE</h1>

  <div class="row">
    <input class="textInput" type="text" id="cmdInput" placeholder="Enter text">
    <div class="secondsWrap">
      <input type="number" id="cmdDuration" min="0.5" step="0.5" value="3">
      <span>S</span>
    </div>
  </div>

  <div class="row">
    <button onclick="sendCustomText()">SEND</button>
    <button onclick="addPreset()">SAVE TO DEVICE</button>
  </div>

  <div class="status" id="status"></div>

  <table>
    <thead>
      <tr>
        <th>TEXT</th>
        <th>TIME</th>
        <th>TYPE</th>
        <th>ACTION</th>
      </tr>
    </thead>
    <tbody id="prescriptRows"></tbody>
  </table>
</div>

<script>
function setStatus(message, isError = false){
  const el = document.getElementById('status');
  el.textContent = message;
  el.style.color = isError ? '#ff8080' : '#99edff';
}

function formatSeconds(ms){
  const seconds = ms / 1000;
  return Number.isInteger(seconds) ? String(seconds) : seconds.toFixed(1);
}

async function requestText(url){
  const res = await fetch(url);
  const text = await res.text();
  if(!res.ok){
    throw new Error(text || 'Request failed');
  }
  return text;
}

async function requestJson(url){
  const res = await fetch(url);
  if(!res.ok){
    throw new Error(await res.text() || 'Request failed');
  }
  return res.json();
}

function renderPrescripts(items){
  const tbody = document.getElementById('prescriptRows');
  tbody.innerHTML = '';

  if(!items.length){
    tbody.innerHTML = '<tr><td colspan="4">No saved text.</td></tr>';
    return;
  }

  for(const item of items){
    const tr = document.createElement('tr');
    tr.innerHTML = `
      <td data-label="Text">${item.text}</td>
      <td data-label="Time">${formatSeconds(item.durationMs)}s</td>
      <td data-label="Type">${item.builtIn ? 'DEFAULT' : 'SAVED'}</td>
      <td data-label="Action">
        <div class="rowActions">
          <input type="number" min="0.5" step="0.5" value="${formatSeconds(item.durationMs)}" id="dur-${item.id}">
          <button onclick="sendPreset(${item.id})">SEND</button>
          ${item.builtIn ? '' : `<button onclick="deletePreset(${item.id})">DELETE</button>`}
        </div>
      </td>
    `;
    tbody.appendChild(tr);
  }
}

async function loadPrescripts(){
  try{
    const items = await requestJson('/prescripts');
    renderPrescripts(items);
  }catch(err){
    setStatus(err.message, true);
  }
}

async function sendCustomText(){
  const text = document.getElementById('cmdInput').value.trim();
  const duration = document.getElementById('cmdDuration').value;

  if(!text){
    setStatus('Enter text first.', true);
    return;
  }

  try{
    await requestText(`/send?val=${encodeURIComponent(text)}&duration=${encodeURIComponent(duration)}`);
    document.getElementById('cmdInput').value = '';
    setStatus('Sent.');
  }catch(err){
    setStatus(err.message, true);
  }
}

async function sendPreset(id){
  const duration = document.getElementById(`dur-${id}`).value;

  try{
    await requestText(`/send?id=${id}&duration=${encodeURIComponent(duration)}`);
    setStatus('Sent.');
  }catch(err){
    setStatus(err.message, true);
  }
}

async function addPreset(){
  const text = document.getElementById('cmdInput').value.trim();
  const duration = document.getElementById('cmdDuration').value;

  if(!text){
    setStatus('Enter text first.', true);
    return;
  }

  try{
    await requestText(`/add-prescript?val=${encodeURIComponent(text)}&duration=${encodeURIComponent(duration)}`);
    document.getElementById('cmdInput').value = '';
    setStatus('Saved.');
    loadPrescripts();
  }catch(err){
    setStatus(err.message, true);
  }
}

async function deletePreset(id){
  try{
    await requestText(`/delete-prescript?id=${id}`);
    setStatus('Deleted.');
    loadPrescripts();
  }catch(err){
    setStatus(err.message, true);
  }
}

loadPrescripts();
</script>
</body>
</html>
)rawliteral";

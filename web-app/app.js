// app.js - FIXED for compartment terminology

async function postJson(path, data) {
  try {
    const res = await fetch(path, {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(data)
    });
    return res.ok ? res.json().catch(()=>null) : Promise.reject(res.status);
  } catch (e) {
    console.error('postJson error', e);
    return null;
  }
}

async function loadList() {
  const listDiv = document.getElementById('list');
  listDiv.innerText = 'Loading...';

  try {
    const r = await fetch('/list');
    if (!r.ok) throw new Error('fetch failed');

    const schedules = await r.json();
    if (!schedules || schedules.length === 0) {
      listDiv.innerText = '(no schedules)';
      return;
    }

    listDiv.innerHTML = '';
    schedules.forEach((s, i) => {
      const d = document.createElement('div');
      d.className = 'item';
      d.innerHTML = `
        <div class="left">
          <div><b>Compartment:</b> ${escapeHtml(s.compartment || '')}</div>
          <div><b>Time:</b> ${escapeHtml(s.time || '')}</div>
          <div><b>Telegram Chat ID:</b> ${escapeHtml(s.telegram || '(none)')}</div>
        </div>
        <div class="right">
          <button class="button danger" onclick="del(${i})">Delete</button>
        </div>
      `;
      listDiv.appendChild(d);
    });

  } catch (e) {
    console.error(e);
    listDiv.innerText = '(failed to load)';
  }
}

async function add() {
  const compartment = document.getElementById('med').value.trim().toUpperCase();
  const time = document.getElementById('time').value;
  const telegram = document.getElementById('tg').value.trim();

  if (!compartment) { 
    alert('Select a compartment (A or B)'); 
    return; 
  }
  
  if (compartment !== 'A' && compartment !== 'B') {
    alert('Compartment must be A or B');
    return;
  }
  
  if (!time) { 
    alert('Select time'); 
    return; 
  }
  
  if (!telegram) {
    alert('Enter Telegram Chat ID');
    return;
  }

  const item = { compartment, time, telegram };
  const res = await postJson('/save', item);

  if (res === null) {
    alert('Save failed');
    return;
  }

  document.getElementById('med').value = '';
  document.getElementById('time').value = '';
  document.getElementById('tg').value = '';

  alert('Schedule saved for Compartment ' + compartment);
  loadList();
}

async function del(index) {
  const ok = confirm('Delete this schedule?');
  if (!ok) return;

  const res = await postJson('/delete', { index });
  if (res === null) {
    alert('Delete failed');
    return;
  }

  loadList();
}

function escapeHtml(text) {
  return String(text)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;');
}

document.getElementById('add').addEventListener('click', add);
document.getElementById('reload').addEventListener('click', loadList);
window.addEventListener('load', loadList);
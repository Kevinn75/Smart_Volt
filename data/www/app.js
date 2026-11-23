let g_config = null;

async function fetchMetrics() {
    try {
        const res = await fetch('/api/data');
        if (!res.ok) throw new Error();
        const d = await res.json();

        document.getElementById('valV').textContent = d.v.toFixed(1);
        document.getElementById('userNameLabel').textContent = d.user;
        document.getElementById('valTotP').textContent = Math.round(d.tot_p) + ' W';
        document.getElementById('valTotI').textContent = d.tot_i.toFixed(2) + ' A';
        document.getElementById('valTotE').textContent = d.tot_e.toFixed(3) + ' kWh';
        document.getElementById('ipLabel').textContent = d.ip;

        const badge = document.getElementById('netStatus');
        badge.textContent = d.status;
        if (d.status.includes('Punto de Acceso')) {
            badge.classList.add('ap');
        } else {
            badge.classList.remove('ap');
        }

        renderChannels(d.channels);
    } catch (e) {
        document.getElementById('netStatus').textContent = 'Vista Previa Local';
        document.getElementById('userNameLabel').textContent = 'Casa de Muestra';
        document.getElementById('valV').textContent = '121.4';
        document.getElementById('valTotP').textContent = '642 W';
        document.getElementById('valTotI').textContent = '5.35 A';
        document.getElementById('valTotE').textContent = '0.245 kWh';
        document.getElementById('ipLabel').textContent = '192.168.4.1 (ESP32)';
        renderChannels([
            { name: "Tablero General (Principal)", i: 4.82, p: 580, s: 585, fp: 0.99, e: 0.210 },
            { name: "Habitación 1", i: 0.53, p: 62, s: 64, fp: 0.97, e: 0.035 },
            { name: "Cocina / Nevera", i: 0.00, p: 0, s: 0, fp: 0.00, e: 0.000 }
        ]);
    }
}

function renderChannels(channels) {
    const grid = document.getElementById('channelsGrid');
    grid.innerHTML = '';

    channels.forEach((ch, idx) => {
        const isMain = (idx === 0);
        const card = document.createElement('div');
        card.className = 'card channel-card' + (isMain ? ' main-room' : '');
        card.innerHTML = `
            <div class="ch-header">
                <h3>${ch.name}</h3>
                <span class="ch-badge">${isMain ? 'Circuito Principal' : 'Habitación ' + idx}</span>
            </div>
            <div class="metric-grid">
                <div class="metric">
                    <span class="lbl">Corriente</span>
                    <span class="val">${ch.i.toFixed(2)} A</span>
                </div>
                <div class="metric">
                    <span class="lbl">Pot. Activa</span>
                    <span class="val accent">${Math.round(ch.p)} W</span>
                </div>
                <div class="metric">
                    <span class="lbl">Pot. Aparente</span>
                    <span class="val">${Math.round(ch.s)} VA</span>
                </div>
                <div class="metric">
                    <span class="lbl">F.P.</span>
                    <span class="val">${ch.fp.toFixed(2)}</span>
                </div>
            </div>
            <div class="energy-footer">
                <span>Energía Acumulada:</span>
                <strong>${ch.e.toFixed(3)} kWh</strong>
            </div>
        `;
        grid.appendChild(card);
    });
}

async function openConfigModal() {
    try {
        const res = await fetch('/api/config');
        if (res.ok) {
            g_config = await res.json();
            document.getElementById('cfgUser').value = g_config.user || '';
            document.getElementById('cfgRooms').value = g_config.rooms || 3;
            document.getElementById('cfgR1').value = g_config.r0 || '';
            document.getElementById('cfgR2').value = g_config.r1 || '';
            document.getElementById('cfgR3').value = g_config.r2 || '';
            document.getElementById('cfgSSID').value = g_config.ssid || '';
            document.getElementById('cfgPass').value = '';
            updateRoomInputs();
        }
    } catch (e) {
        document.getElementById('cfgUser').value = 'Casa de Muestra';
        document.getElementById('cfgRooms').value = '3';
        document.getElementById('cfgR1').value = 'Tablero General (Principal)';
        document.getElementById('cfgR2').value = 'Habitación 1';
        document.getElementById('cfgR3').value = 'Cocina / Nevera';
        document.getElementById('cfgSSID').value = 'Mi_WiFi_Hogar';
    }
    document.getElementById('configModal').style.display = 'flex';
}

function closeConfigModal() {
    document.getElementById('configModal').style.display = 'none';
}

function updateRoomInputs() {
    const rooms = parseInt(document.getElementById('cfgRooms').value);
    document.getElementById('groupRoom2').style.display = (rooms >= 2) ? 'flex' : 'none';
    document.getElementById('groupRoom3').style.display = (rooms >= 3) ? 'flex' : 'none';
}

async function saveConfiguration(e) {
    e.preventDefault();
    const data = {
        user: document.getElementById('cfgUser').value,
        rooms: parseInt(document.getElementById('cfgRooms').value),
        r0: document.getElementById('cfgR1').value,
        r1: document.getElementById('cfgR2').value,
        r2: document.getElementById('cfgR3').value,
        ssid: document.getElementById('cfgSSID').value,
        pass: document.getElementById('cfgPass').value
    };

    try {
        const res = await fetch('/api/config', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify(data)
        });
        if (res.ok) {
            alert('¡Configuración guardada exitosamente!');
            closeConfigModal();
            fetchMetrics();
        } else {
            alert('Error al guardar configuración.');
        }
    } catch (e) {
        alert('¡Configuración simulada en vista previa local! Al estar conectado al ESP32 se guardará en la memoria Flash NVS.');
        closeConfigModal();
    }
}

setInterval(fetchMetrics, 1000);
fetchMetrics();

import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/energy_provider.dart';
import '../models/system_config.dart';

class SettingsTab extends StatefulWidget {
  const SettingsTab({Key? key}) : super(key: key);

  @override
  State<SettingsTab> createState() => _SettingsTabState();
}

class _SettingsTabState extends State<SettingsTab> {
  final _formKey = GlobalKey<FormState>();

  late TextEditingController _userCtrl;
  late TextEditingController _r0Ctrl;
  late TextEditingController _r1Ctrl;
  late TextEditingController _r2Ctrl;
  late TextEditingController _ssidCtrl;
  late TextEditingController _passCtrl;
  late TextEditingController _priceCtrl;

  int _selectedRooms = 3;

  @override
  void initState() {
    super.initState();
    final provider = context.read<EnergyProvider>();
    final cfg = provider.config;

    _userCtrl = TextEditingController(text: cfg.user);
    _r0Ctrl = TextEditingController(text: cfg.r0);
    _r1Ctrl = TextEditingController(text: cfg.r1);
    _r2Ctrl = TextEditingController(text: cfg.r2);
    _ssidCtrl = TextEditingController(text: cfg.ssid);
    _passCtrl = TextEditingController(text: '');
    _priceCtrl = TextEditingController(text: provider.pricePerKwhCop.toStringAsFixed(0));
    _selectedRooms = cfg.rooms;
  }

  @override
  void dispose() {
    _userCtrl.dispose();
    _r0Ctrl.dispose();
    _r1Ctrl.dispose();
    _r2Ctrl.dispose();
    _ssidCtrl.dispose();
    _passCtrl.dispose();
    _priceCtrl.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final provider = context.watch<EnergyProvider>();

    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Form(
        key: _formKey,
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'Configuración General',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 6),
            const Text(
              'Personaliza los circuitos y conecta Smart Volt a tu Wi-Fi',
              style: TextStyle(fontSize: 13, color: Colors.grey),
            ),
            const SizedBox(height: 20),

            // Sección 1: Usuario e Instalación
            _sectionHeader('1. Identificación'),
            TextFormField(
              controller: _userCtrl,
              decoration: const InputDecoration(
                labelText: 'Nombre de la Instalación / Usuario',
                prefixIcon: Icon(Icons.person),
                border: OutlineInputBorder(),
              ),
              validator: (v) => v!.trim().isEmpty ? 'Requerido' : null,
            ),
            const SizedBox(height: 16),

            // Sección 2: Habitaciones / Circuitos
            _sectionHeader('2. Circuitos y Habitaciones Monitoreadas'),
            DropdownButtonFormField<int>(
              value: _selectedRooms,
              decoration: const InputDecoration(
                labelText: 'Cantidad de Circuitos Conectados',
                prefixIcon: Icon(Icons.tune),
                border: OutlineInputBorder(),
              ),
              items: const [
                DropdownMenuItem(value: 1, child: Text('1 Circuito (Solo Tablero Principal)')),
                DropdownMenuItem(value: 2, child: Text('2 Circuitos (Principal + 1 Habitación)')),
                DropdownMenuItem(value: 3, child: Text('3 Circuitos (Principal + 2 Habitaciones)')),
              ],
              onChanged: (val) {
                if (val != null) setState(() => _selectedRooms = val);
              },
            ),
            const SizedBox(height: 12),

            TextFormField(
              controller: _r0Ctrl,
              decoration: const InputDecoration(
                labelText: 'Nombre Canal 1 (Circuito Principal)',
                prefixIcon: Icon(Icons.flash_on),
                border: OutlineInputBorder(),
              ),
            ),
            if (_selectedRooms >= 2) ...[
              const SizedBox(height: 12),
              TextFormField(
                controller: _r1Ctrl,
                decoration: const InputDecoration(
                  labelText: 'Nombre Canal 2 (Habitación 1)',
                  prefixIcon: Icon(Icons.meeting_room),
                  border: OutlineInputBorder(),
                ),
              ),
            ],
            if (_selectedRooms >= 3) ...[
              const SizedBox(height: 12),
              TextFormField(
                controller: _r2Ctrl,
                decoration: const InputDecoration(
                  labelText: 'Nombre Canal 3 (Habitación 2 / Cocina)',
                  prefixIcon: Icon(Icons.kitchen),
                  border: OutlineInputBorder(),
                ),
              ),
            ],
            const SizedBox(height: 20),

            // Sección 3: Conexión Wi-Fi a Internet
            _sectionHeader('3. Red Wi-Fi de Internet'),
            TextFormField(
              controller: _ssidCtrl,
              decoration: const InputDecoration(
                labelText: 'Nombre de Red Wi-Fi (SSID)',
                hintText: 'Tu red Wi-Fi de casa',
                prefixIcon: Icon(Icons.wifi),
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 12),
            TextFormField(
              controller: _passCtrl,
              obscureText: true,
              decoration: const InputDecoration(
                labelText: 'Contraseña Wi-Fi',
                hintText: 'Dejar vacío si no deseas cambiarla',
                prefixIcon: Icon(Icons.lock),
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 20),

            // Sección 4: Parámetros Económicos
            _sectionHeader('4. Tarifa de Energía (Colombia)'),
            TextFormField(
              controller: _priceCtrl,
              keyboardType: TextInputType.number,
              decoration: const InputDecoration(
                labelText: 'Precio por kWh (\$ COP)',
                hintText: 'Ej: 850',
                prefixIcon: Icon(Icons.attach_money),
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 28),

            // Botón Guardar
            SizedBox(
              width: double.infinity,
              height: 50,
              child: ElevatedButton.icon(
                icon: provider.isLoading
                    ? const SizedBox(width: 20, height: 20, child: CircularProgressIndicator(strokeWidth: 2, color: Colors.white))
                    : const Icon(Icons.save),
                label: Text(
                  provider.isLoading ? 'Guardando en Smart Volt...' : 'Guardar y Aplicar Cambios',
                  style: const TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
                ),
                style: ElevatedButton.styleFrom(
                  backgroundColor: Theme.of(context).colorScheme.primary,
                  foregroundColor: Colors.white,
                  shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
                ),
                onPressed: provider.isLoading ? null : () => _saveForm(provider),
              ),
            ),
            const SizedBox(height: 30),
          ],
        ),
      ),
    );
  }

  Widget _sectionHeader(String title) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 10, top: 4),
      child: Text(
        title,
        style: const TextStyle(
          fontSize: 14,
          fontWeight: FontWeight.bold,
          color: Colors.cyanAccent,
        ),
      ),
    );
  }

  Future<void> _saveForm(EnergyProvider provider) async {
    if (!_formKey.currentState!.validate()) return;

    final newConfig = SystemConfigModel(
      user: _userCtrl.text.trim(),
      rooms: _selectedRooms,
      r0: _r0Ctrl.text.trim(),
      r1: _r1Ctrl.text.trim(),
      r2: _r2Ctrl.text.trim(),
      ssid: _ssidCtrl.text.trim(),
      pass: _passCtrl.text.trim(),
    );

    final double price = double.tryParse(_priceCtrl.text.trim()) ?? 850.0;
    await provider.setPricePerKwh(price);

    final success = await provider.saveConfig(newConfig);

    if (mounted) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text(
            success
                ? '¡Configuración guardada en Smart Volt NVS!'
                : 'Configuración guardada localmente (al conectar el ESP32 se aplicará en NVS)',
          ),
          backgroundColor: success ? Colors.green : Colors.blueGrey,
        ),
      );
    }
  }
}

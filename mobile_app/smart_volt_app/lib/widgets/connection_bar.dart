import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/energy_provider.dart';

class ConnectionBar extends StatelessWidget {
  const ConnectionBar({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    final provider = context.watch<EnergyProvider>();
    final isOnline = provider.isConnected;

    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
      color: isOnline
          ? Colors.green.withOpacity(0.12)
          : Colors.amber.withOpacity(0.12),
      child: Row(
        children: [
          Icon(
            isOnline ? Icons.wifi : Icons.wifi_off,
            color: isOnline ? Colors.greenAccent : Colors.amberAccent,
            size: 18,
          ),
          const SizedBox(width: 8),
          Expanded(
            child: Text(
              isOnline
                  ? 'Conectado a Smart Volt (${provider.deviceIp})'
                  : 'Sin conexión (${provider.deviceIp})',
              style: TextStyle(
                color: isOnline ? Colors.greenAccent : Colors.amberAccent,
                fontSize: 12,
                fontWeight: FontWeight.w600,
              ),
              overflow: TextOverflow.ellipsis,
            ),
          ),
          IconButton(
            icon: const Icon(Icons.edit_location_alt, size: 18),
            tooltip: 'Cambiar IP del medidor',
            onPressed: () => _showIpDialog(context, provider),
          ),
          IconButton(
            icon: const Icon(Icons.refresh, size: 18),
            tooltip: 'Reconectar',
            onPressed: () => provider.refreshData(),
          ),
        ],
      ),
    );
  }

  void _showIpDialog(BuildContext context, EnergyProvider provider) {
    final controller = TextEditingController(text: provider.deviceIp);

    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Dirección IP de Smart Volt'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'En modo Punto de Acceso usa 192.168.4.1.\nSi está conectado a tu Wi-Fi, ingresa la IP asignada por tu router.',
              style: TextStyle(fontSize: 13, color: Colors.grey),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: controller,
              decoration: const InputDecoration(
                labelText: 'IP del ESP32',
                hintText: '192.168.4.1 o 192.168.1.xxx',
                border: OutlineInputBorder(),
              ),
              keyboardType: TextInputType.text,
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('Cancelar'),
          ),
          ElevatedButton(
            onPressed: () {
              if (controller.text.trim().isNotEmpty) {
                provider.setDeviceIp(controller.text.trim());
              }
              Navigator.pop(ctx);
            },
            child: const Text('Guardar y Conectar'),
          ),
        ],
      ),
    );
  }
}

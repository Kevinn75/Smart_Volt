import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/energy_provider.dart';
import '../models/metering_data.dart';

class RoomsTab extends StatelessWidget {
  const RoomsTab({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    final provider = context.watch<EnergyProvider>();
    final data = provider.currentData;
    final channels = data.channels.take(data.roomsCount).toList();

    return RefreshIndicator(
      onRefresh: () => provider.refreshData(),
      child: ListView.builder(
        padding: const EdgeInsets.all(16),
        itemCount: channels.length,
        itemBuilder: (context, index) {
          final ch = channels[index];
          final isMain = (index == 0);
          return _buildRoomCard(context, ch, isMain, index + 1);
        },
      ),
    );
  }

  Widget _buildRoomCard(BuildContext context, ChannelData ch, bool isMain, int index) {
    // Estimación de carga sobre breaker residencial estándar de 15 A o 20 A
    final maxBreakerAmps = isMain ? 40.0 : 15.0;
    final loadPercentage = (ch.current / maxBreakerAmps).clamp(0.0, 1.0);

    // Color del factor de potencia
    Color fpColor = Colors.greenAccent;
    String fpLabel = 'Excelente (Resistivo)';
    if (ch.powerFactor < 0.85 && ch.activePower > 20) {
      fpColor = Colors.redAccent;
      fpLabel = 'Bajo (Inductivo)';
    } else if (ch.powerFactor < 0.95 && ch.activePower > 20) {
      fpColor = Colors.amberAccent;
      fpLabel = 'Aceptable';
    }

    return Card(
      elevation: 3,
      margin: const EdgeInsets.only(bottom: 16),
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(18)),
      color: const Color(0xFF1E293B),
      child: Padding(
        padding: const EdgeInsets.all(18),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Cabecera con nombre y tipo
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Row(
                  children: [
                    Container(
                      padding: const EdgeInsets.all(8),
                      decoration: BoxDecoration(
                        color: (isMain ? Colors.cyan : Colors.blueGrey).withOpacity(0.2),
                        borderRadius: BorderRadius.circular(10),
                      ),
                      child: Icon(
                        isMain ? Icons.home : Icons.meeting_room,
                        color: isMain ? Colors.cyanAccent : Colors.white70,
                        size: 20,
                      ),
                    ),
                    const SizedBox(width: 10),
                    Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(
                          ch.name,
                          style: const TextStyle(
                            fontSize: 16,
                            fontWeight: FontWeight.bold,
                          ),
                        ),
                        Text(
                          isMain ? 'Circuito General (Total)' : 'Canal $index • SCT-013',
                          style: const TextStyle(fontSize: 11, color: Colors.grey),
                        ),
                      ],
                    ),
                  ],
                ),
                Chip(
                  label: Text(
                    '${(loadPercentage * 100).toStringAsFixed(0)}% Carga',
                    style: const TextStyle(fontSize: 11, fontWeight: FontWeight.bold),
                  ),
                  backgroundColor: _getLoadColor(loadPercentage).withOpacity(0.15),
                  labelStyle: TextStyle(color: _getLoadColor(loadPercentage)),
                  side: BorderSide.none,
                ),
              ],
            ),
            const SizedBox(height: 14),

            // Barra de Carga sobre capacidad del Breaker
            ClipRRect(
              borderRadius: BorderRadius.circular(4),
              child: LinearProgressIndicator(
                value: loadPercentage,
                minHeight: 6,
                backgroundColor: Colors.grey[800],
                valueColor: AlwaysStoppedAnimation<Color>(_getLoadColor(loadPercentage)),
              ),
            ),
            const SizedBox(height: 16),

            // Grid de 4 Métricas del Circuito
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                _metricItem('Corriente RMS', '${ch.current.toStringAsFixed(2)} A', Colors.white),
                _metricItem('Potencia Activa', '${ch.activePower.toStringAsFixed(0)} W', Colors.orangeAccent),
                _metricItem('Potencia Aparente', '${ch.apparentPower.toStringAsFixed(0)} VA', Colors.white70),
                _metricItem('Factor Potencia', ch.powerFactor.toStringAsFixed(2), fpColor),
              ],
            ),
            const Divider(height: 24),

            // Pie de tarjeta con Energía Acumulada
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Row(
                  children: [
                    const Icon(Icons.speed, size: 16, color: Colors.grey),
                    const SizedBox(width: 6),
                    Text(
                      fpLabel,
                      style: TextStyle(fontSize: 12, color: fpColor),
                    ),
                  ],
                ),
                Text(
                  '${ch.energyKwh.toStringAsFixed(3)} kWh',
                  style: const TextStyle(
                    fontSize: 14,
                    fontWeight: FontWeight.bold,
                    color: Colors.greenAccent,
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _metricItem(String label, String value, Color color) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(
          label,
          style: const TextStyle(fontSize: 11, color: Colors.grey),
        ),
        const SizedBox(height: 2),
        Text(
          value,
          style: TextStyle(
            fontSize: 16,
            fontWeight: FontWeight.bold,
            color: color,
          ),
        ),
      ],
    );
  }

  Color _getLoadColor(double pct) {
    if (pct < 0.6) return Colors.greenAccent;
    if (pct < 0.85) return Colors.amberAccent;
    return Colors.redAccent;
  }
}

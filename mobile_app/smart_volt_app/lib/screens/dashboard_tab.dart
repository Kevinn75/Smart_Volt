import 'package:flutter/material.dart';
import 'package:intl/intl.dart';
import 'package:provider/provider.dart';
import '../providers/energy_provider.dart';
import '../widgets/metric_card.dart';

class DashboardTab extends StatelessWidget {
  const DashboardTab({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    final provider = context.watch<EnergyProvider>();
    final data = provider.currentData;
    final currencyFormat = NumberFormat.currency(locale: 'es_CO', symbol: '\$', decimalDigits: 0);

    // Estado de la tensión: normal (114V - 126V, ±5%)
    final isVoltageNormal = data.voltage >= 110.0 && data.voltage <= 128.0;

    return RefreshIndicator(
      onRefresh: () => provider.refreshData(),
      child: SingleChildScrollView(
        physics: const AlwaysScrollableScrollPhysics(),
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Saludo y Usuario
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      data.user,
                      style: const TextStyle(
                        fontSize: 22,
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                    const Text(
                      'Monitoreo Eléctrico Residencial',
                      style: TextStyle(fontSize: 13, color: Colors.grey),
                    ),
                  ],
                ),
                Chip(
                  avatar: Icon(
                    isVoltageNormal ? Icons.check_circle : Icons.warning_amber,
                    color: isVoltageNormal ? Colors.greenAccent : Colors.amberAccent,
                    size: 16,
                  ),
                  label: Text(
                    isVoltageNormal ? 'Red Estable' : 'Tensión Anormal',
                    style: TextStyle(
                      fontSize: 12,
                      color: isVoltageNormal ? Colors.greenAccent : Colors.amberAccent,
                    ),
                  ),
                  backgroundColor: (isVoltageNormal ? Colors.green : Colors.amber).withOpacity(0.12),
                  side: BorderSide.none,
                ),
              ],
            ),
            const SizedBox(height: 16),

            // Hero Card: Tensión Principal (120 V / 60 Hz)
            Card(
              elevation: 4,
              shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(20)),
              color: const Color(0xFF1E293B),
              child: Container(
                decoration: BoxDecoration(
                  borderRadius: BorderRadius.circular(20),
                  gradient: LinearGradient(
                    colors: [
                      const Color(0xFF1E293B),
                      Colors.cyan.withOpacity(0.15),
                    ],
                    begin: Alignment.topLeft,
                    end: Alignment.bottomRight,
                  ),
                ),
                padding: const EdgeInsets.all(22),
                child: Column(
                  children: [
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: const [
                        Text(
                          'TENSIÓN DE RED',
                          style: TextStyle(
                            fontSize: 13,
                            letterSpacing: 1.2,
                            fontWeight: FontWeight.w600,
                            color: Colors.cyanAccent,
                          ),
                        ),
                        Text(
                          '60.0 Hz • Monofásica',
                          style: TextStyle(fontSize: 12, color: Colors.grey),
                        ),
                      ],
                    ),
                    const SizedBox(height: 12),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      crossAxisAlignment: CrossAxisAlignment.baseline,
                      textBaseline: TextBaseline.alphabetic,
                      children: [
                        Text(
                          data.voltage.toStringAsFixed(1),
                          style: const TextStyle(
                            fontSize: 64,
                            fontWeight: FontWeight.w800,
                            color: Colors.white,
                          ),
                        ),
                        const SizedBox(width: 8),
                        const Text(
                          'VAC',
                          style: TextStyle(
                            fontSize: 24,
                            fontWeight: FontWeight.w600,
                            color: Colors.cyanAccent,
                          ),
                        ),
                      ],
                    ),
                    const SizedBox(height: 10),
                    Text(
                      'ZMPT101B • ADC1_CH0 (GPIO1)',
                      style: TextStyle(fontSize: 12, color: Colors.grey[400]),
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 16),

            // Métricas Principales en Cuadrícula 2x2
            GridView.count(
              crossAxisCount: 2,
              shrinkWrap: true,
              physics: const NeverScrollableScrollPhysics(),
              crossAxisSpacing: 12,
              mainAxisSpacing: 12,
              childAspectRatio: 1.45,
              children: [
                MetricCard(
                  title: 'Potencia Activa Total',
                  value: data.totalPower.toStringAsFixed(0),
                  unit: 'W',
                  icon: Icons.bolt,
                  color: Colors.orangeAccent,
                  subtitle: 'Suma instantánea',
                ),
                MetricCard(
                  title: 'Corriente Total',
                  value: data.totalCurrent.toStringAsFixed(2),
                  unit: 'A',
                  icon: Icons.speed,
                  color: Colors.lightBlueAccent,
                  subtitle: '3 transformadores SCT',
                ),
                MetricCard(
                  title: 'Energía Acumulada',
                  value: data.totalEnergy.toStringAsFixed(3),
                  unit: 'kWh',
                  icon: Icons.electric_meter,
                  color: Colors.greenAccent,
                  subtitle: 'Persistida en NVS',
                ),
                MetricCard(
                  title: 'Habitaciones Activas',
                  value: '${data.roomsCount}',
                  unit: '/ 3',
                  icon: Icons.meeting_room,
                  color: Colors.purpleAccent,
                  subtitle: 'Circuitos habilitados',
                ),
              ],
            ),
            const SizedBox(height: 16),

            // Tarjeta de Estimación Económica (Factura en COP)
            Card(
              elevation: 2,
              shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(16)),
              color: const Color(0xFF1E293B),
              child: Padding(
                padding: const EdgeInsets.all(18),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        Row(
                          children: const [
                            Icon(Icons.monetization_on, color: Colors.greenAccent, size: 20),
                            SizedBox(width: 8),
                            Text(
                              'Estimación de Factura Eléctrica',
                              style: TextStyle(fontSize: 14, fontWeight: FontWeight.bold),
                            ),
                          ],
                        ),
                        Text(
                          '${provider.pricePerKwhCop.toStringAsFixed(0)} COP/kWh',
                          style: const TextStyle(fontSize: 12, color: Colors.grey),
                        ),
                      ],
                    ),
                    const Divider(height: 24),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceBetween,
                      children: [
                        Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            const Text(
                              'Gasto acumulado actual:',
                              style: TextStyle(fontSize: 12, color: Colors.grey),
                            ),
                            const SizedBox(height: 4),
                            Text(
                              currencyFormat.format(provider.estimatedCostCop),
                              style: const TextStyle(
                                fontSize: 20,
                                fontWeight: FontWeight.bold,
                                color: Colors.greenAccent,
                              ),
                            ),
                          ],
                        ),
                        Column(
                          crossAxisAlignment: CrossAxisAlignment.end,
                          children: [
                            const Text(
                              'Proyección mensual:',
                              style: TextStyle(fontSize: 12, color: Colors.grey),
                            ),
                            const SizedBox(height: 4),
                            Text(
                              currencyFormat.format(provider.estimatedMonthlyCostCop),
                              style: const TextStyle(
                                fontSize: 20,
                                fontWeight: FontWeight.bold,
                                color: Colors.amberAccent,
                              ),
                            ),
                          ],
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }
}

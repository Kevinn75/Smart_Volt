import 'dart:async';
import 'package:flutter/foundation.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../models/metering_data.dart';
import '../models/system_config.dart';
import '../services/api_service.dart';

class EnergyProvider with ChangeNotifier {
  final ApiService _apiService = ApiService();
  MeteringData _currentData = MeteringData.mock();
  SystemConfigModel _config = SystemConfigModel();

  bool _isConnected = false;
  bool _isLoading = false;
  String _errorMessage = '';
  Timer? _pollingTimer;

  // Historial de potencia total de los últimos 60 puntos para gráficos en tiempo real
  final List<double> _powerHistory = [];
  final int _maxHistoryPoints = 60;

  // Tarifa eléctrica en COP / kWh (configurable por el usuario)
  double _pricePerKwhCop = 850.0;

  MeteringData get currentData => _currentData;
  SystemConfigModel get config => _config;
  bool get isConnected => _isConnected;
  bool get isLoading => _isLoading;
  String get errorMessage => _errorMessage;
  List<double> get powerHistory => _powerHistory;
  double get pricePerKwhCop => _pricePerKwhCop;
  String get deviceIp => _apiService.baseUrl.replaceAll('http://', '').replaceAll('https://', '');

  EnergyProvider() {
    _loadPreferences();
    startPolling();
  }

  Future<void> _loadPreferences() async {
    final prefs = await SharedPreferences.getInstance();
    final savedIp = prefs.getString('device_ip') ?? '192.168.4.1';
    _pricePerKwhCop = prefs.getDouble('price_kwh_cop') ?? 850.0;
    _apiService.updateBaseUrl(savedIp);
    notifyListeners();
  }

  Future<void> setDeviceIp(String ip) async {
    _apiService.updateBaseUrl(ip);
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString('device_ip', ip);
    await refreshData();
  }

  Future<void> setPricePerKwh(double price) async {
    _pricePerKwhCop = price;
    final prefs = await SharedPreferences.getInstance();
    await prefs.setDouble('price_kwh_cop', price);
    notifyListeners();
  }

  void startPolling() {
    _pollingTimer?.cancel();
    _pollingTimer = Timer.periodic(const Duration(seconds: 1), (timer) {
      _pollData();
    });
  }

  void stopPolling() {
    _pollingTimer?.cancel();
  }

  Future<void> _pollData() async {
    try {
      final data = await _apiService.fetchMeteringData();
      _currentData = data;
      _isConnected = true;
      _errorMessage = '';

      // Agregar a historial de potencia para gráfica
      _powerHistory.add(data.totalPower);
      if (_powerHistory.length > _maxHistoryPoints) {
        _powerHistory.removeAt(0);
      }
    } catch (e) {
      // Si falla la conexión, mantenemos la app viva y marcamos desconectado
      _isConnected = false;
      _errorMessage = 'No se puede conectar al ESP32 ($deviceIp)';
    }
    notifyListeners();
  }

  Future<void> refreshData() async {
    _isLoading = true;
    notifyListeners();
    await _pollData();
    await fetchConfig();
    _isLoading = false;
    notifyListeners();
  }

  Future<void> fetchConfig() async {
    try {
      final cfg = await _apiService.fetchConfiguration();
      _config = cfg;
      notifyListeners();
    } catch (_) {}
  }

  Future<bool> saveConfig(SystemConfigModel newConfig) async {
    _isLoading = true;
    notifyListeners();
    final ok = await _apiService.saveConfiguration(newConfig);
    if (ok) {
      _config = newConfig;
    }
    _isLoading = false;
    notifyListeners();
    return ok;
  }

  // Costo total acumulado estimado en pesos colombianos
  double get estimatedCostCop {
    return _currentData.totalEnergy * _pricePerKwhCop;
  }

  // Proyección de costo mensual estimado (asumiendo potencia promedio actual)
  double get estimatedMonthlyCostCop {
    // Potencia en kW * 24 horas * 30 días * tarifa
    final kw = _currentData.totalPower / 1000.0;
    return kw * 24 * 30 * _pricePerKwhCop;
  }

  @override
  void dispose() {
    _pollingTimer?.cancel();
    super.dispose();
  }
}

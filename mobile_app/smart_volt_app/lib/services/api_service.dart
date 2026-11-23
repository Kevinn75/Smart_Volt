import 'dart:convert';
import 'package:http/http.dart' as http;
import '../models/metering_data.dart';
import '../models/system_config.dart';

class ApiService {
  String baseUrl;

  ApiService({this.baseUrl = 'http://192.168.4.1'});

  void updateBaseUrl(String newUrl) {
    if (!newUrl.startsWith('http://') && !newUrl.startsWith('https://')) {
      baseUrl = 'http://$newUrl';
    } else {
      baseUrl = newUrl;
    }
    // Quitar barra final si la tiene
    if (baseUrl.endsWith('/')) {
      baseUrl = baseUrl.substring(0, baseUrl.length - 1);
    }
  }

  Future<MeteringData> fetchMeteringData() async {
    final url = Uri.parse('$baseUrl/api/data');
    final response = await http.get(url).timeout(const Duration(seconds: 3));

    if (response.statusCode == 200) {
      final json = jsonDecode(response.body);
      return MeteringData.fromJson(json);
    } else {
      throw Exception('Fallo HTTP ${response.statusCode}');
    }
  }

  Future<SystemConfigModel> fetchConfiguration() async {
    final url = Uri.parse('$baseUrl/api/config');
    final response = await http.get(url).timeout(const Duration(seconds: 3));

    if (response.statusCode == 200) {
      final json = jsonDecode(response.body);
      return SystemConfigModel.fromJson(json);
    } else {
      throw Exception('Fallo al obtener configuración');
    }
  }

  Future<bool> saveConfiguration(SystemConfigModel config) async {
    final url = Uri.parse('$baseUrl/api/config');
    final body = jsonEncode(config.toJson());

    final response = await http.post(
      url,
      headers: {'Content-Type': 'application/json'},
      body: body,
    ).timeout(const Duration(seconds: 5));

    return response.statusCode == 200;
  }
}

#include <WiFiS3.h>
#include <WiFiUdp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <NTPClient.h>
#include "arduino_secrets.h"

#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

WiFiServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000);
int status = WL_IDLE_STATUS;
float ultimaTemperatura = 0.0;
char ultimaTimestamp[25];

float lerTemperatura();
void atualizarHorario();

void setup() {
  Serial.begin(9600);
  tempSensor.begin();
  tempSensor.setResolution(12);

  while (status != WL_CONNECTED) {
    Serial.print("Conectando ao WiFi: ");
    Serial.println(SECRET_SSID);
    status = WiFi.begin(SECRET_SSID, SECRET_PASS);
    delay(5000);
  }

  Serial.println("WiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  timeClient.update();

  server.begin();
}

void loop() {
  ultimaTemperatura = lerTemperatura();
  atualizarHorario();

  WiFiClient client = server.available();
  if (client) {
    String req = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        req += c;
        if (c == '\n' && req.endsWith("\r\n\r\n")) break;
      }
    }

    if (req.indexOf("GET /api/temperature") >= 0) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();
      client.print("{\"temperature\":");
      client.print(ultimaTemperatura, 1);
      client.print(",\"timestamp\":\"");
      client.print(ultimaTimestamp);
      client.println("\"}");
    } else {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();
      client.println("<!DOCTYPE html><html><head><meta charset='UTF-8'>");
      client.println("<title>Grafico de Temperatura</title>");
      client.println("<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>");
      client.println("<script src='https://cdn.jsdelivr.net/npm/xlsx/dist/xlsx.full.min.js'></script>");
      client.println("<style>body{font-family:sans-serif;text-align:center;margin:30px}canvas{max-width:90%;margin-bottom:20px;}button{margin:5px;padding:10px 20px;font-size:16px}</style>");
      client.println("</head><body>");
      client.println("<h2>Temperatura DS18B20</h2>");
      client.println("<canvas id='grafico'></canvas>");
      client.println("<div id='status' style='font-weight:bold;margin:10px;'></div>");
      client.println("<button onclick='exportarCSV()'>Exportar CSV</button>");
      client.println("<button onclick='exportarXLSX()'>Exportar XLSX</button>");
      client.println("<script>");
      client.println("const maxPontos = 100, tempData = [], tempLabels = [];");
      client.println("let ultimoTempo = Date.now();");

      client.println("const ctx = document.getElementById(\"grafico\").getContext(\"2d\");");
      client.println("const chart = new Chart(ctx, {type: \"line\",data: {labels: tempLabels,datasets: [{label: \"Temperatura (C)\",borderColor: \"red\",data: tempData,fill: false}]},options: {scales: {y: {beginAtZero: false},x: {title: {display: true,text: \"Data e Hora\"}}}}});");

      client.println("async function atualizar() {");
      client.println("  try {");
      client.println("    const r = await fetch(\"/api/temperature\");");
      client.println("    const json = await r.json();");
      client.println("    tempLabels.push(json.timestamp);");
      client.println("    tempData.push(parseFloat(json.temperature).toFixed(1));");
      client.println("    if (tempLabels.length > maxPontos) { tempLabels.shift(); tempData.shift(); }");
      client.println("    chart.update();");
      client.println("    ultimoTempo = Date.now();");
      client.println("    document.getElementById(\"status\").innerText = \"Recebendo dados\";");
      client.println("    document.getElementById(\"status\").style.color = \"green\";");
      client.println("  } catch (e) {");
      client.println("    document.getElementById(\"status\").innerText = \"Erro na leitura\";");
      client.println("    document.getElementById(\"status\").style.color = \"red\";");
      client.println("  }");
      client.println("}");

      client.println("function verificarTimeout() {");
      client.println("  const agora = Date.now();");
      client.println("  if (agora - ultimoTempo > 10000) {");
      client.println("    document.getElementById(\"status\").innerText = \"Sem leitura\";");
      client.println("    document.getElementById(\"status\").style.color = \"orange\";");
      client.println("  }");
      client.println("}");

      client.println("function exportarCSV() {");
      client.println("  let csv = 'Data e Hora,Temperatura (C)\\n';");
      client.println("  for(let i=0;i<tempData.length;i++){");
      client.println("    csv += tempLabels[i]+','+parseFloat(tempData[i]).toFixed(1)+'\\n';");
      client.println("  }");
      client.println("  const blob = new Blob([csv], {type: 'text/csv;charset=utf-8;'});");
      client.println("  const url = URL.createObjectURL(blob);");
      client.println("  const a = document.createElement('a');");
      client.println("  a.href = url;");
      client.println("  a.download = 'temperatura.csv';");
      client.println("  a.click();");
      client.println("}");

      client.println("function exportarXLSX() {");
      client.println("  const dados = [[\"Data e Hora\", \"Temperatura (C)\"]];");
      client.println("  for(let i=0;i<tempData.length;i++){");
      client.println("    dados.push([tempLabels[i], parseFloat(tempData[i]).toFixed(1)]);");
      client.println("  }");
      client.println("  const worksheet = XLSX.utils.aoa_to_sheet(dados);");
      client.println("  const workbook = XLSX.utils.book_new();");
      client.println("  XLSX.utils.book_append_sheet(workbook, worksheet, \"Temperaturas\");");
      client.println("  XLSX.writeFile(workbook, \"temperatura.xlsx\");");
      client.println("}");

      client.println("setInterval(atualizar, 5000);");
      client.println("setInterval(verificarTimeout, 3000);");
      client.println("atualizar();");
      client.println("</script></body></html>");
    }

    client.stop();
  }

  delay(100);
}

float lerTemperatura() {
  tempSensor.requestTemperatures();
  float t = tempSensor.getTempCByIndex(0);
  if (t == DEVICE_DISCONNECTED_C) {
    Serial.println("Erro: sensor desconectado!");
    return NAN;
  }
  return t;
}

void atualizarHorario() {
  timeClient.update();
  unsigned long epoch = timeClient.getEpochTime();
  int sec = epoch % 60;
  int min = (epoch / 60) % 60;
  int hour = (epoch / 3600) % 24;

  int days = epoch / 86400;
  int year = 1970;
  int month = 1;
  int day = 1;

  while (days >= 365) {
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
      if (days >= 366) {
        days -= 366;
        year++;
      } else break;
    } else {
      days -= 365;
      year++;
    }
  }

  const int daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  for (int i = 0; i < 12; i++) {
    int dim = daysInMonth[i];
    if (i == 1 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) dim = 29;
    if (days >= dim) {
      days -= dim;
      month++;
    } else break;
  }
  day += days;

  sprintf(ultimaTimestamp, "%02d/%02d/%04d %02d:%02d:%02d", day, month, year, hour, min, sec);
}

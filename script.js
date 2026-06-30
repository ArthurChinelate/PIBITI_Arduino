
const SHEET_ID = '0';

// Temperatura: aba com A = timestamp, B = temperatura (cabeçalho na linha 1)
const TEMP_SHEET_NAME = 'Dados';

// pH: C = data/hora; D..última = amostras (cabeçalho na linha 1)
const PH_SHEET_NAME = 'Dados';
const PH_TIMESTAMP_COL = 3;        // C
const PH_FIRST_SERIES_COL = 4;     // D


function doGet(e) {
  return HtmlService.createHtmlOutputFromFile('index')
    .setTitle('IoT Carnes')
    .setXFrameOptionsMode(HtmlService.XFrameOptionsMode.ALLOWALL);
}

/** Temperatura */
function getTemperatures(options) {
  options = options || {};
  const maxRows = options.maxRows || 50000;

  const ss = SpreadsheetApp.openById(SHEET_ID);
  const sh = ss.getSheetByName(TEMP_SHEET_NAME);
  if (!sh) return [];

  const lastRow = sh.getLastRow();
  const lastCol = sh.getLastColumn();
  if (lastRow < 2) return [];

  const rows = Math.min(lastRow - 1, maxRows);
  const values = sh.getRange(2, 1, rows, Math.min(2, lastCol)).getDisplayValues();

  return values
    .map(r => ({ t: r[0], temp: r[1] }))
    .filter(o => String(o.t).trim() !== '' && String(o.temp).trim() !== '');
}

/** pH: retorna séries [{name, data:[{t, v}]}] */
function getPhSeries(options) {
  options = options || {};
  const maxRows = options.maxRows || 50000;

  const ss = SpreadsheetApp.openById(SHEET_ID);
  const sh = ss.getSheetByName(PH_SHEET_NAME);
  if (!sh) return { series: [] };

  const lastRow = sh.getLastRow();
  const lastCol = sh.getLastColumn();
  if (lastRow < 2) return { series: [] };

  const rows = Math.min(lastRow - 1, maxRows);

  // Cabeçalhos das séries (D..)
  const headerRange = sh.getRange(1, PH_FIRST_SERIES_COL, 1, lastCol - PH_FIRST_SERIES_COL + 1);
  const headers = headerRange.getDisplayValues()[0];

  let seriesCount = headers.length;
  while (seriesCount > 0 && String(headers[seriesCount - 1]).trim() === '') seriesCount--;
  if (seriesCount <= 0) return { series: [] };

  const tVals = sh.getRange(2, PH_TIMESTAMP_COL, rows, 1).getDisplayValues();
  const phVals = sh.getRange(2, PH_FIRST_SERIES_COL, rows, seriesCount).getDisplayValues();

  const series = [];
  for (let s = 0; s < seriesCount; s++) {
    const name = String(headers[s] || `Série ${s + 1}`).trim();
    const data = [];
    for (let r = 0; r < rows; r++) {
      const t = tVals[r][0];
      const v = phVals[r][s];
      if (String(t).trim() !== '' && String(v).trim() !== '') data.push({ t, v });
    }
    series.push({ name, data });
  }
  return { series };
}

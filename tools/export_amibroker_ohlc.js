var args = WScript.Arguments;

if (args.length < 1) {
    WScript.Echo("Usage: export_amibroker_ohlc.js <output.csv>");
    WScript.Quit(1);
}

var outputPath = args(0);
var fso = new ActiveXObject("Scripting.FileSystemObject");

/* Connect directly to AmiBroker; no Rev5 CSV dependency. */
var AB = new ActiveXObject("Broker.Application");

WScript.Echo("AmiBroker Version : " + AB.Version);
WScript.Echo("Database          : " + AB.DatabasePath);
WScript.Echo("Stocks            : " + AB.Stocks.Count);

var out = fso.CreateTextFile(outputPath, true);
out.WriteLine("symbol,date,open,high,low,close,volume");

var exportedSymbols = 0;
var skippedSymbols = 0;
var failedSymbols = 0;
var totalBars = 0;

for (var s = 0; s < AB.Stocks.Count; s++) {
    try {
        var stk = AB.Stocks.Item(s);
        if (!stk) {
            failedSymbols++;
            continue;
        }

        var symbol = stk.Ticker;
        if (!symbol || symbol.length == 0) {
            skippedSymbols++;
            continue;
        }

        symbol = symbol.replace(/^\s+|\s+$/g, "");
        symbol = symbol.toUpperCase();

        var count = stk.Quotations.Count;
        if (count <= 0) {
            skippedSymbols++;
            continue;
        }

        var start = Math.max(0, count - 120);
        var bars = [];

        for (var i = start; i < count; i++) {
            var q = stk.Quotations(i);
            var d = new Date(q.Date);
            var yyyy = d.getFullYear();
            var mm = ("0" + (d.getMonth() + 1)).slice(-2);
            var dd = ("0" + d.getDate()).slice(-2);

            bars.push({
                date: yyyy + "-" + mm + "-" + dd,
                open: q.Open,
                high: q.High,
                low: q.Low,
                close: q.Close,
                volume: q.Volume
            });
        }

        for (var b = 0; b < bars.length; b++) {
            out.WriteLine(
                symbol + "," +
                bars[b].date + "," +
                bars[b].open + "," +
                bars[b].high + "," +
                bars[b].low + "," +
                bars[b].close + "," +
                bars[b].volume
            );
            totalBars++;
        }

        exportedSymbols++;
        if (exportedSymbols <= 20 || exportedSymbols % 100 == 0) {
            var last = bars[bars.length - 1];
            WScript.Echo(
                exportedSymbols + " | " + symbol +
                " | bars=" + bars.length +
                " | latest=" + last.date +
                " | close=" + last.close
            );
        }
    }
    catch (e) {
        failedSymbols++;
        if (failedSymbols <= 20) {
            WScript.Echo("ERROR index=" + s + " | " + e.message);
        }
    }
}

out.Close();

WScript.Echo("");
WScript.Echo("========== OHLC EXPORT COMPLETE ==========");
WScript.Echo("AmiBroker Stocks : " + AB.Stocks.Count);
WScript.Echo("Symbols exported : " + exportedSymbols);
WScript.Echo("Symbols skipped  : " + skippedSymbols);
WScript.Echo("Symbols failed   : " + failedSymbols);
WScript.Echo("Total bars       : " + totalBars);
WScript.Echo("Output           : " + outputPath);

if (exportedSymbols == 0) {
    WScript.Quit(2);
}
WScript.Quit(0);

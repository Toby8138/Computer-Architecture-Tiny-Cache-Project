# Computer-Architecture-Tiny-Cache-Project
This contains the content of a Cache written with system c which is part of a group project of the module Practical course Computer Architecture at the Technical University of Munich


# Einführung in das Caching
Ein **Cache** ist ein kleiner, nahe der CPU platzierter Speicher. Er basiert auf der Annahme, dass kürzlich genutzte Daten mit hoher Wahrscheinlichkeit bald erneut benötigt werden.

Moderne Cache-Architekturen bestehen typischerweise aus mehreren **Cache-Ebenen** (in der Regel drei). Jede Ebene stellt dabei einen eigenständigen Speicherbereich mit unterschiedlicher **Größe** und **Zugriffszeit (Latenz)** dar.

Jede Cache-Ebene besteht aus sogenannten **Cache-Lines**, der kleinsten Speichereinheit in diesem Kontext. Eine Cache-Line beinhaltet üblicherweise:
- **Die tatsächlichen Daten** (ein Block aus dem Hauptspeicher),
- **Ein Tag** (zur Identifikation, welcher Speicherblock gespeichert ist),
- **Ein Valid-Bit** (zur Kennzeichnung, ob die Daten gültig/nutzbar sind).

Beim Laden von Daten in den Cache definiert die **Mapping-Strategie**, wie Speicherblöcke in den Cache eingeordnet werden:
- **Direct-Mapped**: Jeder Speicherblock wird genau einer Cache-Line zugeordnet. Dies führt häufig zu Kollisionen.
- **Fully Associative**: Jeder Speicherblock kann in jeder beliebigen Cache-Line gespeichert werden. Diese Methode minimiert Kollisionen, ist jedoch komplexer in der Umsetzung.
- **Set-Associative**: Der Cache wird in Sets unterteilt, wobei jedes Set mehrere Cache-Lines enthält. Speicherblöcke werden genau einem Set zugewiesen und innerhalb dieses Sets entsprechend der gewählten Strategie gespeichert.

# Forschungsfragen
## Typische Größen von Caches, Cache-Lines und Zugriffszeiten moderner CPUs:
Frühere Prozessoren verfügten oft nur über eine einstufige Cache-Struktur mit 16KB oder 32KB. Heutige Systeme nutzen in der Regel **drei Cache-Ebenen** mit einer Gesamtkapazität von bis zu **1,15 GB**. Dennoch bestehen Cache-Lines üblicherweise aus **64 Byte**. Die Charakteristika der Ebenen sind wie folgt:
| Cache-Ebene | Größe                 | Latenz (Zyklen)|
|-------------|-----------------------|----------------|
| L1          | 16KB bis 32KB         | ~1–3           |
| L2          | 256KB bis mehrere MB  | ~3–10          |
| L3          | mehrere bis >10 MB    | ~10–20+        |

## Ersetzungsstrategien und deren Vor- und Nachteile
- **LRU (Least Recently Used)**: Entfernt das am längsten nicht genutzte Element.
    + *Vorteile*: Einfach, gute durchschnittliche Leistung.
    + *Nachteile*: Schwach bei Zugriffen auf große Datenmengen in Schleifen.
- **LFU (Least Frequently Used)**: Entfernt das am wenigsten häufig genutzte Element.
    + *Vorteile*: Optimal bei „heißen“ (häufig genutzten) Daten.
    + *Nachteile*: Nicht adaptiv, alte heiße Werte verbleiben zu lange im Cache.
- **FIFO (First-In First-Out)**: Entfernt das älteste eingefügte Element.
    + *Vorteile*: Einfach, kein zusätzlicher Verwaltungsaufwand.
    + *Nachteile*: Berücksichtigt nicht die tatsächliche Nutzung.
- **Random Replacement**: Entfernt ein zufällig ausgewähltes Element.
    + *Vorteile*: Umgeht Worst-Case-Szenarien anderer Strategien.
    + *Nachteile*: Nicht deterministisch, kann wichtige Daten entfernen.
- **MRU (Most Recently Used)**: Entfernt das zuletzt genutzte Element.
    + *Vorteile*: Geeignet für Backtracking-Algorithmen.
    + *Nachteile*: Im Widerspruch zu typischen Zugriffsmustern.
- **Belady’s Algorithmus**: Entfernt das Element, das am längsten nicht benötigt wird.
    + *Vorteile*: Theoretisch optimale Strategie.
    + *Nachteile*: In der Praxis nicht umsetzbar, da zukünftiger Bedarf bekannt sein müsste.

## Speicherzugriffsverhalten speicherintensiver Algorithmen
Der Matrixmultiplikationsalgorithmus erzeugt eine X*Z-Matrix *C* aus einer X*Y-Matrix *A* und einer Y*Z-Matrix *B* mithilfe von drei verschachtelten Schleifen über die Indizes **i, j und k**. In der naiven Version des Algorithmus erfordert jede innere Aktualisierung von C[i][j]+=A[i][k]*B[k][j] vier Speicherzugriffe:
- **Lesen** A[i][k]
- **Lesen** B[k][j]
- **Lesen** C[i][j]
- **Schreiben** C[i][j]
Matrizen werden üblicherweise als zusammenhängende Speicherblöcke gespeichert, können aber entweder **column-major oder row-major** sein. Je nach Speicherart können wir die Dimensionen **zeilen- oder spaltenorientiert** durchlaufen. Durch die richtige Schleifenführung ist der Algorithmus optimal auf das Caching abgestimmt, da wir stets in der Reihenfolge auf die Matrixelemente zugreifen, in der sie gespeichert sind. Mit anderen Worten: Der Algorithmus weist eine hohe Lokalität auf.

# Funktionsweise des Cache-Moduls
Das **Cache-Modul** verwendet Strukturen zur Repräsentation von **Cache-Ebenen** und **Cache-Lines**. Dies ermöglicht Modularität und eine einfache Erstellung sehr unterschiedlicher Cache-Konfigurationen. Unterstützt werden die Strategien **LRU**, **LFU**, **FIFO**, **MRU** sowie **zufällige Zuordnungen**.

## Lese- und Schreibvorgänge
Lese- und Schreibzugriffe folgen einem ähnlichen Ablauf. Da jede Adresse ein Byte speichert, wird der Zugriff auf 32/8 = 4 Byte gesplittet und einzeln behandelt.
- **Lesezugriff**: Bei einem Cache-Treffer wird die zugehörige Cache-Line auch in allen niedrigeren Ebenen hinzugefügt. Bei einem Cache-Miss erfolgt ein Zugriff auf den Hauptspeicher, das Erstellen einer neuen Cache-Line, deren Einfügen in alle Ebenen und das Rückführen des angeforderten Bytes.
- **Schreibzugriff**: Bei einem Cache-Treffer wird das neue Byte geschrieben, alle höheren Ebenen werden aktualisiert und anschließend der Hauptspeicher. Bei einem Cache-Miss erfolgt ein direkter Hauptspeicherzugriff sowie das Nachladen in alle relevanten Cache-Lines.

In beiden Fällen werden Signale gesetzt, um die erreichte Ebene zu dokumentieren und die **simulierte Latenz** korrekt zu erfassen.

## Cache-Ebene
Die Cache-Ebene stellt vier Methoden zur Kommunikation mit dem Cache-Modul bereit:
- **hasAddress**: Prüft, ob eine gegebene Adresse in der Cache-Ebene vorhanden ist. Dazu wird das entsprechende Set bestimmt und nach dem passenden Tag durchsucht.
- **load**: Lädt eine neue Cache-Line in das passende Set unter Berücksichtigung der Assoziativität und Mapping-Strategie.
- **read_line**: Ruft die Cache-Line ab, die die angegebene Adresse enthält.
- **write_byte**: Schreibt ein einzelnes Byte in die Cache-Line mit der entsprechenden Adresse.
- **get_line_data_by_index**: Helfer für getCacheLineContent

## Cache-Line
Speichert eine Byte-Sequenz entsprechend der Größe der Cache-Line. Stellt die notwendigen Verwaltungsvariablen bereit, die für die unterschiedlichen Mapping-Strategien erforderlich sind.


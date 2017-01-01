Param(
[string]$File,
[string]$Output,
[string]$VarName
)

$fin = New-Object System.IO.BinaryReader([System.IO.File]::Open($File, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite))
$fout = New-Object System.IO.StreamWriter($Output)
$fout.NewLine = "`n"

$fout.WriteLine("const char {0}[] = {{" -f $VarName);

$pos = 0;
while($true)
{
    $c = $fin.Read();
    if($c -lt 0) { break; }
    if($pos -ne 0) { $fout.Write(","); }
    if($c -eq '`r') { continue; }

    $fout.Write("0x{0:X2}" -f $c);
    $pos+=1;
    if($pos % 20 -eq 0) { $fout.Write("`n"); }
}

$fout.WriteLine("};");
$fout.WriteLine("");
$fout.WriteLine($("unsigned const int {0}_size = {1};" -f $VarName, $pos));

$fin.Close()
$fout.Close()
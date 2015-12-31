use File::Copy qw(copy);
use File::Copy qw(move);
$daily_build_path ="W:\\iot-host-gp";
$daily_result_path ="d:\\dailyReport";
$test_file_path = "";
$test_file ="";

opendir(DIR,$daily_build_path) or die "Couldn't  open directory $daily_build_path $!";
@files=readdir(DIR);
closedir(DIR);

foreach $gpfile(@files)
{
    if(-M "$daily_build_path\\$gpfile" <1)
    { 
      print "$gpfile\n";
      $test_7zfile_path = "$daily_build_path\\$gpfile";
      
      $test_file =  substr $gpfile,0, length($gpfile)-3;
      print "$test_file\n";
    }
}

system("c:\\Progra~1\\7-zip\\7z.exe x $test_7zfile_path -oc:\\daily-build-gp");


#run sikuli
system("c:\\tools\\sikuli\\runIDE.cmd -r c:\\tools\\autoTest.sikuli >autoTest.txt");
#delete test file
system("RD c:\\daily-build-gp\\$test_file /S /Q");
$test_file =  $test_file."-all";
system("mkdir $daily_result_path\\$test_file");

opendir(ResDIR,$daily_result_path) or die "Couldn't  open directory $daily_result_path $!";
@Rfiles=readdir(ResDIR);
closedir(ResDIR);

foreach $resultFile(@Rfiles)
{
		$checkPath = "$daily_result_path\\$resultFile";
    if(-d $checkPath)
    { }
    else
    {
      system("move $checkPath $daily_result_path\\$test_file");
      sleep(10);
    }
}
system("move $daily_result_path\\$test_file v:\\");

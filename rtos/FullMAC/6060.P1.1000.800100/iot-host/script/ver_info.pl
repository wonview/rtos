#!/usr/bin/perl
use strict;

my @tmp_string;
my $svn_status;
my $svn_filename;
my $svn_version;
my $sline;

my $svn_root = ".";
my $svn_root_version;
my $version_file = './version.txt';


sub get_version_linux {
    foreach (@_) {
        if($_ =~ m/^Last Changed Rev: (\d*)/) {
            return $1;
        }
    } 
    # file doesn't exist on svn
    return -1;
}


sub get_version_win {
    foreach (@_) {
        if($_ =~ m/^Last committed at revision (\d*)/) {
            return $1;
        }
    } 
    # file doesn't exist on svn
    return -1;
}


printf("## script to generate version infomation header ##\n");

# step-0: get root svn number 

if($^O eq "linux")
{
 
  $svn_root_version = get_version_linux(qx(svn info $svn_root)); 
  if($svn_root_version == -1) 
  {
    print "aa\n";
    if(-e "$version_file")
    {
      if(open(verfile,$version_file))
      {
        while($sline = <verfile>)
        {
             $svn_root_version = $sline;
             chomp $svn_root_version;
        }
        close(verfile);        
      }
    }
  }
}
else
{  
   $svn_root_version = get_version_win(qx("subwcrev $svn_root"));
}

OUTPUT_HEADER:
# step-3: output header files
if (defined($ARGV[0])) {
    open HEADER, ">", $ARGV[0];
    select HEADER;
}
else {
    print "Error! Please specify source file\n";
}

##
printf("const char *version = { \"%d\"};\n\n", $svn_root_version);

use POSIX qw(strftime);
my $date = strftime "%Y/%m/%d/ %H:%M", localtime;
printf("const char *date 	= { \"$date\" };\n");
##


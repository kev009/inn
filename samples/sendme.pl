# $Id$
# Copyright (c)1998 G.J. Andruk
# sendme.pl - The sendme control message.
#  Parameters: params sender reply-to token site action[=log] approved
sub control_sendme {
  my $artfh;
  
  my @params = split(/\s+/,shift);
  my $sender = shift;
  my $replyto = shift;
  my $token = shift;
  my $site = shift;
  my ($action, $logging) = split(/=/, shift);
  my $approved = shift;
  
  my $pid = $$;
  my $tempfile = "$inn::tmpdir/sendme.$pid";
  
  my ($errmsg, $status, $nc, @component, @oldgroup, $locktry,
      $ngname, $ngdesc, $modcmd, $kid);
  
  if ($action eq "mail") {
    $artfh = open_article($token);
    next if (!defined($artfh));
    *ARTICLE = $artfh;
    
  SMHEAD: while(<ARTICLE>) { last SMHEAD if /^$/ }
    $kid = open2(\*R, \*MAIL, @inn::mailcmd, "-s",
	  "sendme by $sender", $inn::newsmaster);
    while (<ARTICLE>) {
      s/^~/~~/;
      print MAIL $_;
    }
    close ARTICLE;
    close R;
    close MAIL;
    waitpid($kid, 0);
  } elsif ($action eq "log") {
    if (!$logging) {
      logmsg ('notice', 'sendme from %s', $sender);
    } else {
      logger($token, $logging, "sendme $sender");
    }
  } elsif ($action eq "doit") {
    open(GREPHIST, "|grephistory -s > $tempfile");
    
    $artfh = open_article($token);
    next if (!defined($artfh));
    *ARTICLE = $artfh;
    
  SMHEAD2: while(<ARTICLE>) { last SMHEAD2 if /^$/ }
    print GREPHIST $_ while <ARTICLE>;  
    close(ARTICLE);
    close(GREPHIST);
    ## xxx Need to lock the work file?
    if (-s $tempfile && ($site =~ /(^[a-zA-Z0-9.-_]+$)/)) {
      open(TEMPFILE, "<$tempfile");
      open(BATCH, ">>$inn::batch/$1.work");
      print BATCH $_ while <TEMPFILE>;  
      close(BATCH);
      close(TEMPFILE);
    }
    unlink($tempfile);
  }
}

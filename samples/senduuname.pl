# $Id$
# Copyright (c)1998 G.J. Andruk
# senduuname.pl - The senduuname control message.
#  Parameters: params sender reply-to token site action[=log] approved
sub control_senduuname {
  my $artfh;
  
  my @params = split(/\s+/,shift);
  my $sender = shift;
  my $replyto = shift;
  my $token = shift;
  my $site = shift;
  my ($action, $logging) = split(/=/, shift);
  my $approved = shift;
  
  my $groupname = $params[0];
  
  my $pid = $$;
  my $tempfile = "$inn::tmpdir/senduuname.$pid";
  
  my ($errmsg, $status, $nc, @component, @oldgroup, $locktry,
      $ngname, $ngdesc, $modcmd, $kid);
  
  if ($action eq "mail") {
    open (TEMPFILE, ">$tempfile");
    print TEMPFILE ("$sender has requested information about your\n",
		    "UUCP name.\n\n",
		    "If this is acceptable, type:\n",
		    "  uuname | ",
		    "$inn::mailcmd -s \"senduuname reply from ",
		    "$inn::pathhost\" $replyto\n\n",
		    "The control message follows:\n\n");
    
    $artfh = open_article($token);
    next if (!defined($artfh));
    *ARTICLE = $artfh;
    
    print TEMPFILE $_ while <ARTICLE>;  
    close(ARTICLE);
    close(TEMPFILE);
    logger($tempfile, "mail", "senduuname $sender\n");
    unlink($tempfile);
  } elsif ($action eq "log") {
    if (!$logging) {
      logmsg ('notice', 'senduuname %s', $sender);
    } else {
      logger($token, $logging, "senduuname $sender");
    }
  } elsif ($action =~ /^(doit|doifarg)$/) {
    if (($action eq "doifarg") && ($params[0] ne $inn::pathhost)) {
      logmsg ('notice', 'skipped senduuname %s', $sender);
    } else {
      open UUNAME, "uuname|";
      $kid = open2 (\*R, \*MAIL, $inn::mailcmd, "-s",
	     "senduuname reply from $inn::pathhost", $replyto);
      print MAIL $_ while <UUNAME>;  
      close UUNAME;
      close R;
      close MAIL;
      waitpid($kid, 0);
      if ($logging) {
	$errmsg = "senduuname $sender to $replyto";
	logger($token, $logging, $errmsg);
      }
    }
  }
}

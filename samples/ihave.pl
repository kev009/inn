# $Id$
# Copyright (c)1998 G.J. Andruk
# ihave.pl - The ihave control message.
#  Parameters: params sender reply-to token site action[=log] approved
sub control_ihave {
  my $artfh;
  
  my @params = split(/\s+/,shift);
  my $sender = shift;
  my $replyto = shift;
  my $token = shift;
  my $site = shift;
  my ($action, $logging) = split(/=/, shift);
  my $approved = shift;
  
  my $pid = $$;
  my $tempfile = "$inn::tmpdir/ihave.$pid";
  
  my ($errmsg, $status, $nc, @component, @oldgroup, $locktry,
      $ngname, $ngdesc, $modcmd, $kid);
  
  if ($action eq "mail") {
    
    $artfh = open_article($token);
    next if (!defined($artfh));
    *ARTICLE = $artfh;
    
  IHHEAD:
    while (<ARTICLE>) {
      last IHHEAD if /^$/;
    }
    $kid = open2(\*R, \*MAIL, @inn::mailcmd, "-s",
	  "ihave by $sender", $inn::newsmaster);
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
      logmsg ('notice', 'ihave %s', $sender);
    } else {
      logger($token, $logging, "ihave $sender");
    }
  } elsif ($action eq "doit") {
    
    open(GREPHIST, "|grephistory -i > $tempfile");
    $artfh = open_article($token);
    next if (!defined($artfh));
    *ARTICLE = $artfh;
    
  IHHEAD2:
    while (<ARTICLE>) {
      last IHHEAD2 if /^$/;
    }
    print GREPHIST $_ while <ARTICLE>;  
    close(ARTICLE);
    close(GREPHIST);
    
    if (-s "$tempfile") {
      $kid = open2(\*R, \*INEWS, $inn::inews, "-h");
      print INEWS ("Newsgroups: to.$site\n",
		   "Subject: cmsg sendme $inn::pathhost\n",
		   "Control: sendme $inn::pathhost\n\n");
      open TEMPFILE, "<$tempfile";
      print INEWS $_ while <TEMPFILE>;  
      close R;
      close INEWS;
      close TEMPFILE;
      waitpid($kid, 0);
    }
    unlink ($tempfile);
  }
}
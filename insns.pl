#!/usr/bin/perl
# 
# insns.pl   produce insnsa.c, insnsd.c, insnsi.h, insnsn.c from insns.dat
#
# The Netwide Assembler is copyright (C) 1996 Simon Tatham and
# Julian Hall. All rights reserved. The software is
# redistributable under the licence given in the file "Licence"
# distributed in the NASM archive.

# Opcode prefixes which need their own opcode tables
# LONGER PREFIXES FIRST!
@disasm_prefixes = qw(0F24 0F25 0F38 0F3A 0F7A 0FA6 0FA7 0F);

print STDERR "Reading insns.dat...\n";

@args   = ();
undef $output;
foreach $arg ( @ARGV ) {
    if ( $arg =~ /^\-/ ) {
	if  ( $arg =~ /^\-([adin])$/ ) {
	    $output = $1;
	} else {
	    die "$0: Unknown option: ${arg}\n";
	}
    } else {
	push (@args, $arg);
    }
}

$fname = "insns.dat" unless $fname = $args[0];
open (F, $fname) || die "unable to open $fname";

%dinstables = ();

$line = 0;
$insns = 0;
while (<F>) {
  $line++;
  next if /^\s*;/;   # comments
  chomp;
  split;
  next if $#_ == -1; # blank lines
  (warn "line $line does not contain four fields\n"), next if $#_ != 3;
  ($formatted, $nd) = &format(@_);
  if ($formatted) {
    $insns++;
    $aname = "aa_$_[0]";
    push @$aname, $formatted;
  }
  if ( $_[0] =~ /cc$/ ) {
      # Conditional instruction
      $k_opcodes_cc{$_[0]}++;
  } else {
      # Unconditional instruction
      $k_opcodes{$_[0]}++;
  }
  if ($formatted && !$nd) {
    push @big, $formatted;
    foreach $i (startseq($_[2])) {
	if (!defined($dinstables{$i})) {
	    $dinstables{$i} = [];
	}
	push(@{$dinstables{$i}}, $#big);
    }
  }
}

close F;

@opcodes    = sort keys(%k_opcodes);
@opcodes_cc = sort keys(%k_opcodes_cc);

if ( !defined($output) || $output eq 'a' ) {
    print STDERR "Writing insnsa.c...\n";
    
    open A, ">insnsa.c";
    
    print A "/* This file auto-generated from insns.dat by insns.pl" .
        " - don't edit it */\n\n";
    print A "#include \"insns.h\"\n";
    print A "\n";
    
    foreach $i (@opcodes, @opcodes_cc) {
	print A "static const struct itemplate instrux_${i}[] = {\n";
	$aname = "aa_$i";
	foreach $j (@$aname) {
	    print A "    $j\n";
	}
	print A "    ITEMPLATE_END\n};\n\n";
    }
    print A "const struct itemplate * const nasm_instructions[] = {\n";
    foreach $i (@opcodes, @opcodes_cc) {
	print A "    instrux_${i},\n";
    }
    print A "};\n";
    
    close A;
}


printf STDERR "Done: %d instructions\n", $insns;

sub format {
    my ($opcode, $operands, $codes, $flags) = @_;
    my $num, $nd = 0;

    return (undef, undef) if $operands eq "ignore";
    
    # format the operands
    $operands =~ s/:/|colon,/g;
    $operands =~ s/mem(\d+)/mem|bits$1/g;
    $operands =~ s/mem/memory/g;
    $operands =~ s/memory_offs/mem_offs/g;
    $operands =~ s/imm(\d+)/imm|bits$1/g;
    $operands =~ s/imm/immediate/g;
    $operands =~ s/rm(\d+)/rm_gpr|bits$1/g;
    $operands =~ s/mmxrm/rm_mmx/g;
    $operands =~ s/xmmrm/rm_xmm/g;
    $operands =~ s/\=([0-9]+)/same_as|$1/g;
    if ($operands eq 'void') {
	@ops = ();
    } else {
	@ops = split(/\,/, $operands);
    }
    $num = scalar(@ops);
    while (scalar(@ops) < 4) {
	push(@ops, '0');
    }
    $operands = join(',', @ops);
    $operands =~ tr/a-z/A-Z/;
    
    # format the flags
    $flags =~ s/,/|IF_/g;
    $flags =~ s/(\|IF_ND|IF_ND\|)//, $nd = 1 if $flags =~ /IF_ND/;
    $flags = "IF_" . $flags;
    
    ("{I_$opcode, $num, {$operands}, \"$codes\", $flags, \"$opcode\"},", $nd);
}

sub hexlist($$$) {
    my($prefix, $start, $n) = @_;
    my $i;
    my @l = ();

    for ($i = 0; $i < $n; $i++) {
	push(@l, sprintf("%s%02X", $prefix, $start+$i));
    }
    return @l;
}

# Here we determine the range of possible starting bytes for a given
# instruction. We need only consider the codes:
# \1 \2 \3     mean literal bytes, of course
# \4 \5 \6 \7  mean PUSH/POP of segment registers: special case
# \1[0123]     mean byte plus register value
# \170         means byte zero
# \330         means byte plus condition code
# \0 or \340   mean give up and return empty set
sub startseq($) {
  my ($codestr) = @_;
  my $word, @range;
  my @codes = ();
  my $c = $codestr;
  my $c0, $c1, $i;
  my $prefix = '';

  # Although these are C-syntax strings, by convention they should have
  # only octal escapes (for directives) and hexadecimal escapes
  # (for verbatim bytes)
  while ($c ne '') {
      if ($c =~ /^\\x([0-9a-f]+)(.*)$/i) {
	  push(@codes, hex $1);
	  $c = $2;
	  next;
      } elsif ($c =~ /^\\([0-7]{1,3})(.*)$/) {
	  push(@codes, oct $1);
	  $c = $2;
	  next;
      } else {
	  die "$0: unknown code format in \"$codestr\"\n";
      }
  }

  while ($c0 = shift(@codes)) {
      $c1 = $codes[0];
      if ($c0 == 01 || $c0 == 02 || $c0 == 03 || $c0 == 0170) {
	  # Fixed byte string
	  my $fbs = $prefix;
	  while (1) {
	      if ($c0 == 01 || $c0 == 02 || $c0 == 03) {
		  while ($c0--) {
		      $fbs .= sprintf("%02X", shift(@codes));
		  }
	      } elsif ($c0 == 0170) {
		  $fbs .= '00';
	      } else {
		  last;
	      }
	      $c0 = shift(@codes);
	  }

	  foreach $pfx (@disasm_prefixes) {
	      if ($fbs =~ /^$pfx(.*)$/) {
		  $prefix = $pfx;
		  $fbs = $1;
		  last;
	      }
	  }

	  if ($fbs ne '') {
	      return ($prefix.substr($fbs,0,2));
	  }
      } elsif ($c0 == 04) {
	  return ("07", "17", "1F");
      } elsif ($c0 == 05) {
	  return ("A1", "A9");
      } elsif ($c0 == 06) {
	  return ("06", "0E", "16", "1E");
      } elsif ($c0 == 07) {
	  return ("A0", "A8");
      } elsif ($c0 >= 010 && $c0 <= 013) {
	  return hexlist($prefix, $c1, 8);
      } elsif ($c0 == 0330) {
	  return hexlist($prefix, $c1, 16);
      } elsif ($c0 == 0 || $c0 == 0340) {
	  return ();
      }
  }
  return ();
}

#! /usr/bin/perl -w

#    Copyright (C) 1998, 1999 Tom Tromey
#    Copyright (C) 2001 Red Hat Software

#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2, or (at your option)
#    any later version.

#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.

#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
#    02111-1307, USA.

# Contributer(s):
#   Andrew Taylor <andrew.taylor@montage.ca>

# gen-unicode-tables.pl - Generate tables for libunicode from Unicode data.
# See http://www.unicode.org/Public/UNIDATA/UnicodeCharacterDatabase.html
# Usage: gen-unicode-tables.pl [-decomp | -both] UNICODE-VERSION UnicodeData.txt LineBreak.txt SpecialCasing.txt CaseFolding.txt
# I consider the output of this program to be unrestricted.  Use it as
# you will.

# FIXME:
# * For decomp table it might make sense to use a shift count other
#   than 8.  We could easily compute the perfect shift count.

use vars qw($CODE $NAME $CATEGORY $COMBINING_CLASSES $BIDI_CATEGORY $DECOMPOSITION $DECIMAL_VALUE $DIGIT_VALUE $NUMERIC_VALUE $MIRRORED $OLD_NAME $COMMENT $UPPER $LOWER $TITLE $BREAK_CODE $BREAK_CATEGORY $BREAK_NAME $CASE_CODE $CASE_LOWER $CASE_TITLE $CASE_UPPER $CASE_CONDITION);

# Names of fields in Unicode data table.
$CODE = 0;
$NAME = 1;
$CATEGORY = 2;
$COMBINING_CLASSES = 3;
$BIDI_CATEGORY = 4;
$DECOMPOSITION = 5;
$DECIMAL_VALUE = 6;
$DIGIT_VALUE = 7;
$NUMERIC_VALUE = 8;
$MIRRORED = 9;
$OLD_NAME = 10;
$COMMENT = 11;
$UPPER = 12;
$LOWER = 13;
$TITLE = 14;

# Names of fields in the line break table
$BREAK_CODE = 0;
$BREAK_PROPERTY = 1;

# Names of fields in the SpecialCasing table
$CASE_CODE = 0;
$CASE_LOWER = 1;
$CASE_TITLE = 2;
$CASE_UPPER = 3;
$CASE_CONDITION = 4;

# Names of fields in the CaseFolding table
$FOLDING_CODE = 0;
$FOLDING_STATUS = 1;
$FOLDING_MAPPING = 2;

# Map general category code onto symbolic name.
%mappings =
    (
     # Normative.
     'Lu' => "G_UNICODE_UPPERCASE_LETTER",
     'Ll' => "G_UNICODE_LOWERCASE_LETTER",
     'Lt' => "G_UNICODE_TITLECASE_LETTER",
     'Mn' => "G_UNICODE_NON_SPACING_MARK",
     'Mc' => "G_UNICODE_COMBINING_MARK",
     'Me' => "G_UNICODE_ENCLOSING_MARK",
     'Nd' => "G_UNICODE_DECIMAL_NUMBER",
     'Nl' => "G_UNICODE_LETTER_NUMBER",
     'No' => "G_UNICODE_OTHER_NUMBER",
     'Zs' => "G_UNICODE_SPACE_SEPARATOR",
     'Zl' => "G_UNICODE_LINE_SEPARATOR",
     'Zp' => "G_UNICODE_PARAGRAPH_SEPARATOR",
     'Cc' => "G_UNICODE_CONTROL",
     'Cf' => "G_UNICODE_FORMAT",
     'Cs' => "G_UNICODE_SURROGATE",
     'Co' => "G_UNICODE_PRIVATE_USE",
     'Cn' => "G_UNICODE_UNASSIGNED",

     # Informative.
     'Lm' => "G_UNICODE_MODIFIER_LETTER",
     'Lo' => "G_UNICODE_OTHER_LETTER",
     'Pc' => "G_UNICODE_CONNECT_PUNCTUATION",
     'Pd' => "G_UNICODE_DASH_PUNCTUATION",
     'Ps' => "G_UNICODE_OPEN_PUNCTUATION",
     'Pe' => "G_UNICODE_CLOSE_PUNCTUATION",
     'Pi' => "G_UNICODE_INITIAL_PUNCTUATION",
     'Pf' => "G_UNICODE_FINAL_PUNCTUATION",
     'Po' => "G_UNICODE_OTHER_PUNCTUATION",
     'Sm' => "G_UNICODE_MATH_SYMBOL",
     'Sc' => "G_UNICODE_CURRENCY_SYMBOL",
     'Sk' => "G_UNICODE_MODIFIER_SYMBOL",
     'So' => "G_UNICODE_OTHER_SYMBOL"
     );

%break_mappings =
    (
     'BK' => "G_UNICODE_BREAK_MANDATORY",
     'CR' => "G_UNICODE_BREAK_CARRIAGE_RETURN",
     'LF' => "G_UNICODE_BREAK_LINE_FEED",
     'CM' => "G_UNICODE_BREAK_COMBINING_MARK",
     'SG' => "G_UNICODE_BREAK_SURROGATE",
     'ZW' => "G_UNICODE_BREAK_ZERO_WIDTH_SPACE",
     'IN' => "G_UNICODE_BREAK_INSEPARABLE",
     'GL' => "G_UNICODE_BREAK_NON_BREAKING_GLUE",
     'CB' => "G_UNICODE_BREAK_CONTINGENT",
     'SP' => "G_UNICODE_BREAK_SPACE",
     'BA' => "G_UNICODE_BREAK_AFTER",
     'BB' => "G_UNICODE_BREAK_BEFORE",
     'B2' => "G_UNICODE_BREAK_BEFORE_AND_AFTER",
     'HY' => "G_UNICODE_BREAK_HYPHEN",
     'NS' => "G_UNICODE_BREAK_NON_STARTER",
     'OP' => "G_UNICODE_BREAK_OPEN_PUNCTUATION",
     'CL' => "G_UNICODE_BREAK_CLOSE_PUNCTUATION",
     'QU' => "G_UNICODE_BREAK_QUOTATION",
     'EX' => "G_UNICODE_BREAK_EXCLAMATION",
     'ID' => "G_UNICODE_BREAK_IDEOGRAPHIC",
     'NU' => "G_UNICODE_BREAK_NUMERIC",
     'IS' => "G_UNICODE_BREAK_INFIX_SEPARATOR",
     'SY' => "G_UNICODE_BREAK_SYMBOL",
     'AL' => "G_UNICODE_BREAK_ALPHABETIC",
     'PR' => "G_UNICODE_BREAK_PREFIX",
     'PO' => "G_UNICODE_BREAK_POSTFIX",
     'SA' => "G_UNICODE_BREAK_COMPLEX_CONTEXT",
     'AI' => "G_UNICODE_BREAK_AMBIGUOUS",
     'XX' => "G_UNICODE_BREAK_UNKNOWN"
     );

# Title case mappings.
%title_to_lower = ();
%title_to_upper = ();

# Maximum length of special-case strings

my $special_case_len = 0;
my @special_cases;

$do_decomp = 0;
$do_props = 1;
if (@ARGV && $ARGV[0] eq '-decomp')
{
    $do_decomp = 1;
    $do_props = 0;
    shift @ARGV;
}
elsif (@ARGV && $ARGV[0] eq '-both')
{
    $do_decomp = 1;
    shift @ARGV;
}

if (@ARGV != 6) {
    $0 =~ s@.*/@@;
    die "Usage: $0 [-decomp | -both] UNICODE-VERSION UnicodeData.txt LineBreak.txt SpecialCasing.txt CaseFolding.txt CompositionExclusions.txt\n";
}
 
print "Creating decomp table\n" if ($do_decomp);
print "Creating property table\n" if ($do_props);

print "Composition exlusions from $ARGV[5]\n";

open (INPUT, "< $ARGV[5]") || exit 1;

while (<INPUT>) {

    chop;

    next if /^#/;
    next if /^\s*$/;

    s/\s*#.*//;

    s/^\s*//;
    s/\s*$//;

    $composition_exclusions{hex($_)} = 1;
}

close INPUT;

print "Unicode data from $ARGV[1]\n";

open (INPUT, "< $ARGV[1]") || exit 1;

$last_code = -1;
while (<INPUT>)
{
    chop;
    @fields = split (';', $_, 30);
    if ($#fields != 14)
    {
	printf STDERR ("Entry for $fields[$CODE] has wrong number of fields (%d)\n", $#fields);
    }

    $code = hex ($fields[$CODE]);

    last if ($code > 0xFFFF); # ignore characters out of the basic plane

    if ($code > $last_code + 1)
    {
	# Found a gap.
	if ($fields[$NAME] =~ /Last>/)
	{
	    # Fill the gap with the last character read,
            # since this was a range specified in the char database
	    @gfields = @fields;
	}
	else
	{
	    # The gap represents undefined characters.  Only the type
	    # matters.
	    @gfields = ('', '', 'Cn', '0', '', '', '', '', '', '', '',
			'', '', '', '');
	}
	for (++$last_code; $last_code < $code; ++$last_code)
	{
	    $gfields{$CODE} = sprintf ("%04x", $last_code);
	    &process_one ($last_code, @gfields);
	}
    }
    &process_one ($code, @fields);
    $last_code = $code;
}

close INPUT;

@gfields = ('', '', 'Cn', '0', '', '', '', '', '', '', '',
	    '', '', '', '');
for (++$last_code; $last_code < 0x10000; ++$last_code)
{
    $gfields{$CODE} = sprintf ("%04x", $last_code);
    &process_one ($last_code, @gfields);
}
--$last_code;			# Want last to be 0xFFFF.

print "Creating line break table\n";

print "Line break data from $ARGV[2]\n";

open (INPUT, "< $ARGV[2]") || exit 1;

$last_code = -1;
while (<INPUT>)
{
    my ($start_code, $end_code);
    
    chop;

    next if /^#/;

    s/\s*#.*//;
    
    @fields = split (';', $_, 30);
    if ($#fields != 1)
    {
	printf STDERR ("Entry for $fields[$CODE] has wrong number of fields (%d)\n", $#fields);
	next;
    }

    if ($fields[$CODE] =~ /([A-F0-9]{4})..([A-F0-9]{4})/) 
    {
	$start_code = hex ($1);
	$end_code = hex ($2);
    } else {
	$start_code = $end_code = hex ($fields[$CODE]);
	
    }

    last if ($start_code > 0xFFFF); # FIXME ignore characters out of the basic plane 

    if ($start_code > $last_code + 1)
    {
	# The gap represents undefined characters. If assigned,
	# they are AL, if not assigned, XX
	for (++$last_code; $last_code < $start_code; ++$last_code)
	{
	    if ($type[$last_code] eq 'Cn')
	    {
		$break_props[$last_code] = 'XX';
	    }
	    else
	    {
		$break_props[$last_code] = 'AL';
	    }
	}
    }

    for ($last_code = $start_code; $last_code <= $end_code; $last_code++)
    {
	$break_props[$last_code] = $fields[$BREAK_PROPERTY];
    }
    
    $last_code = $end_code;
}

close INPUT;

for (++$last_code; $last_code < 0x10000; ++$last_code)
{
  if ($type[$last_code] eq 'Cn')
    {
      $break_props[$last_code] = 'XX';
    }
  else
    {
      $break_props[$last_code] = 'AL';
    }
}
--$last_code;			# Want last to be 0xFFFF.

print STDERR "Last code is not 0xFFFF" if ($last_code != 0xFFFF);

print "Reading special-casing table for case conversion\n";

open (INPUT, "< $ARGV[3]") || exit 1;

while (<INPUT>)
{
    my $code;
    
    chop;

    next if /^#/;
    next if /^\s*$/;

    s/\s*#.*//;

    @fields = split ('\s*;\s*', $_, 30);

    $raw_code = $fields[$CASE_CODE];
    $code = hex ($raw_code);

    if ($#fields != 4 && $#fields != 5)
    {
	printf STDERR ("Entry for $raw_code has wrong number of fields (%d)\n", $#fields);
	next;
    }

    if (!defined $type[$code])
    {
	printf STDERR "Special case for code point: $code, which has no defined type\n";
	next;
    }

    if (defined $fields[5]) {
	# Ignore conditional special cases - we'll handle them in code
	next;
    }
    
    if ($type[$code] eq 'Lu') 
    {
	(hex $fields[$CASE_UPPER] == $code) || die "$raw_code is Lu and UCD_Upper($raw_code) != $raw_code";

	&add_special_case ($code, $value[$code],$fields[$CASE_LOWER], $fields[$CASE_TITLE]);
	
    } elsif ($type[$code] eq 'Lt') 
    {
	(hex $fields[$CASE_TITLE] == $code) || die "$raw_code is Lt and UCD_Title($raw_code) != $raw_code";
	
	&add_special_case ($code, undef,$fields[$CASE_LOWER], $fields[$CASE_UPPER]);
    } elsif ($type[$code] eq 'Ll') 
    {
	(hex $fields[$CASE_LOWER] == $code) || die "$raw_code is Ll and UCD_Lower($raw_code) != $raw_code";
	
	&add_special_case ($code, $value[$code],$fields[$CASE_UPPER], $fields[$CASE_TITLE]);
    } else {
	printf STDERR "Special case for non-alphabetic code point: $raw_code\n";
	next;
    }
}

close INPUT;

open (INPUT, "< $ARGV[4]") || exit 1;

my $casefoldlen = 0;
my @casefold;
 
while (<INPUT>)
{
    my $code;
    
    chop;

    next if /^#/;
    next if /^\s*$/;

    s/\s*#.*//;

    @fields = split ('\s*;\s*', $_, 30);

    $raw_code = $fields[$FOLDING_CODE];
    $code = hex ($raw_code);

    next if $code > 0xffff;	# FIXME!
    
    if ($#fields != 3)
    {
	printf STDERR ("Entry for $raw_code has wrong number of fields (%d)\n", $#fields);
	next;
    }

    next if ($fields[$FOLDING_STATUS] eq 'S');

    @values = map { hex ($_) } split /\s+/, $fields[$FOLDING_MAPPING];

    # Check simple case

    if (@values == 1 && 
	!(defined $value[$code] && $value[$code] >= 0xd800 && $value[$code] < 0xdc00) &&
	defined $type[$code]) {

	my $lower;
	if ($type[$code] eq 'Ll') 
	{
	    $lower = $code;
	} elsif ($type[$code] eq 'Lt') 
	{
	    $lower = $title_to_lower{$code};
	} elsif ($type[$code] eq 'Lu') 
	{
	    $lower = $value[$code];
	} else {
	    $lower = $code;
	}
	
	if ($lower == $values[0]) {
	    next;
	}
    }

    my $string = pack ("U*", @values);
    if (1 + length $string > $casefoldlen) {
	$casefoldlen = 1 + length $string;
    }

    push @casefold, [ $code, $string ];
}

close INPUT;

if ($do_props) {
    &print_tables ($last_code)
}
if ($do_decomp) {
    &print_decomp ($last_code);
    &output_composition_table;
}

&print_line_break ($last_code);

exit 0;

# Process a single character.
sub process_one
{
    my ($code, @fields) = @_;

    $type[$code] = $fields[$CATEGORY];
    if ($type[$code] eq 'Nd')
    {
	$value[$code] = int ($fields[$DECIMAL_VALUE]);
    }
    elsif ($type[$code] eq 'Ll')
    {
	$value[$code] = hex ($fields[$UPPER]);
    }
    elsif ($type[$code] eq 'Lu')
    {
	$value[$code] = hex ($fields[$LOWER]);
    }

    if ($type[$code] eq 'Lt')
    {
	$title_to_lower{$code} = hex ($fields[$LOWER]);
	$title_to_upper{$code} = hex ($fields[$UPPER]);
    }

    $cclass[$code] = $fields[$COMBINING_CLASSES];

    # Handle decompositions.
    if ($fields[$DECOMPOSITION] ne '')
    {
	if ($fields[$DECOMPOSITION] =~ s/\<.*\>\s*//) {
           $decompose_compat[$code] = 1;
	} else {
           $decompose_compat[$code] = 0;

	   if (!exists $composition_exclusions{$code}) {
	       $compositions{$code} = $fields[$DECOMPOSITION];
	   }
        }
	$decompositions[$code] = $fields[$DECOMPOSITION];
    }
}

sub print_tables
{
    my ($last) = @_;
    my ($outfile) = "gunichartables.h";

    local ($bytes_out) = 0;

    print "Writing $outfile...\n";

    open (OUT, "> $outfile");

    print OUT "/* This file is automatically generated.  DO NOT EDIT!\n";
    print OUT "   Instead, edit gen-unicode-tables.pl and re-run.  */\n\n";

    print OUT "#ifndef CHARTABLES_H\n";
    print OUT "#define CHARTABLES_H\n\n";

    print OUT "#define G_UNICODE_DATA_VERSION \"$ARGV[0]\"\n\n";

    printf OUT "#define G_UNICODE_LAST_CHAR 0x%04x\n\n", $last;

    printf OUT "#define G_UNICODE_MAX_TABLE_INDEX 1000\n\n";

    $table_index = 0;
    printf OUT "static const char type_data[][256] = {\n";
    for ($count = 0; $count <= $last; $count += 256)
    {
	$row[$count / 256] = &print_row ($count, 1, \&fetch_type);
    }
    printf OUT "\n};\n\n";

    print OUT "static const short type_table[256] = {\n";
    for ($count = 0; $count <= $last; $count += 256)
    {
	print OUT ",\n" if $count > 0;
	print OUT "  ", $row[$count / 256];
	$bytes_out += 2;
    }
    print OUT "\n};\n\n";


    #
    # Now print attribute table.
    #

    $table_index = 0;
    printf OUT "static const unsigned short attr_data[][256] = {\n";
    for ($count = 0; $count <= $last; $count += 256)
    {
	$row[$count / 256] = &print_row ($count, 2, \&fetch_attr);
    }
    printf OUT "\n};\n\n";

    print OUT "static const short attr_table[256] = {\n";
    for ($count = 0; $count <= $last; $count += 256)
    {
	print OUT ",\n" if $count > 0;
	print OUT "  ", $row[$count / 256];
	$bytes_out += 2;
    }
    print OUT "\n};\n\n";

    #
    # print title case table
    #

    # FIXME: type.
    print OUT "static const unsigned short title_table[][3] = {\n";
    my ($item);
    my ($first) = 1;
    foreach $item (sort keys %title_to_lower)
    {
	print OUT ",\n"
	    unless $first;
	$first = 0;
	printf OUT "  { 0x%04x, 0x%04x, 0x%04x }", $item, $title_to_upper{$item}, $title_to_lower{$item};
	$bytes_out += 6;
    }
    print OUT "\n};\n\n";

    #
    # And special case conversion table -- conversions that change length
    #
    &output_special_case_table (\*OUT);
    &output_casefold_table (\*OUT);

    print OUT "#endif /* CHARTABLES_H */\n";

    close (OUT);

    printf STDERR "Generated %d bytes in tables\n", $bytes_out;
}

# A fetch function for the type table.
sub fetch_type
{
    my ($index) = @_;
    return $mappings{$type[$index]};
}

# A fetch function for the attribute table.
sub fetch_attr
{
    my ($index) = @_;
    if (defined $value[$index])
      {
        return sprintf ("0x%04x", $value[$index]);
      }
    else
      {
        return "0x0000";
      }
}

sub print_row
{
    my ($start, $typsize, $fetcher) = @_;

    my ($i);
    my (@values);
    my ($flag) = 1;
    my ($off);

    for ($off = 0; $off < 256; ++$off)
    {
	$values[$off] = $fetcher->($off + $start);
	if ($values[$off] ne $values[0])
	{
	    $flag = 0;
	}
    }
    if ($flag)
    {
	return $values[0] . " + G_UNICODE_MAX_TABLE_INDEX";
    }

    printf OUT ",\n" if ($table_index != 0);
    printf OUT "  { /* page %d, index %d */\n    ", $start / 256, $table_index;
    my ($column) = 4;
    for ($i = $start; $i < $start + 256; ++$i)
    {
	print OUT ", "
	    if $i > $start;
	my ($text) = $values[$i - $start];
	if (length ($text) + $column + 2 > 78)
	{
	    print OUT "\n    ";
	    $column = 4;
	}
	print OUT $text;
	$column += length ($text) + 2;
    }
    print OUT "\n  }";

    $bytes_out += 256 * $typsize;

    return sprintf "%d /* page %d */", $table_index++, $start / 256;
}

# Generate the character decomposition header.
sub print_decomp
{
    my ($last) = @_;
    my ($outfile) = "gunidecomp.h";

    local ($bytes_out) = 0;

    print "Writing $outfile...\n";

    open (OUT, "> $outfile") || exit 1;

    print OUT "/* This file is automatically generated.  DO NOT EDIT! */\n\n";
    print OUT "#ifndef DECOMP_H\n";
    print OUT "#define DECOMP_H\n\n";

    printf OUT "#define G_UNICODE_LAST_CHAR 0x%04x\n\n", $last;

    printf OUT "#define G_UNICODE_MAX_TABLE_INDEX 1000\n\n";

    my ($count, @row);
    $table_index = 0;
    printf OUT "static const unsigned char cclass_data[][256] = {\n";
    for ($count = 0; $count <= $last; $count += 256)
    {
	$row[$count / 256] = &print_row ($count, 1, \&fetch_cclass);
    }
    printf OUT "\n};\n\n";

    print OUT "static const short combining_class_table[256] = {\n";
    for ($count = 0; $count <= $last; $count += 256)
    {
	print OUT ",\n" if $count > 0;
	print OUT "  ", $row[$count / 256];
	$bytes_out += 2;
    }
    print OUT "\n};\n\n";

    print OUT "typedef struct\n{\n";
    # FIXME: type.
    print OUT "  unsigned short ch;\n";
    print OUT "  unsigned char canon_offset;\n";
    print OUT "  unsigned char compat_offset;\n";
    print OUT "  unsigned char *expansion;\n";
    print OUT "} decomposition;\n\n";

    print OUT "static const decomposition decomp_table[] =\n{\n";
    my ($iter);
    my ($first) = 1;
    for ($count = 0; $count <= $last; ++$count)
    {
	if (defined $decompositions[$count])
	{
	    print OUT ",\n"
		if ! $first;
	    $first = 0;

	    my $canon_decomp;
	    my $compat_decomp;

	    if (!$decompose_compat[$count]) {
		$canon_decomp = make_decomp ($count, 0);
	    }
	    $compat_decomp = make_decomp ($count, 1);

	    if (defined $canon_decomp && $compat_decomp eq $canon_decomp) {
		undef $compat_decomp; 
	    }

	    my $string = "";
	    my $canon_offset = 0xff;
	    my $compat_offset = 0xff;
	    
	    if (defined $canon_decomp) {
		$canon_offset = 0;
		$string .= $canon_decomp;
	    }
	    if (defined $compat_decomp) {
		if (defined $canon_decomp) {
		    $string .= "\\x00\\x00";
		}
		$compat_offset = (length $string) / 4;
		$string .= $compat_decomp;
	    }

	    $bytes_out += (length $string) / 4; # "\x20"
	    
	    # Only a single terminator because one is implied in the string.
	    printf OUT qq(  { 0x%04x, %u, %u, "%s\\0" }), 
                $count, $canon_offset, $compat_offset, $string;
   	        
	    
	    $bytes_out += 6;
	}
    }
    print OUT "\n};\n\n";

    print OUT "#endif /* DECOMP_H */\n";

    printf STDERR "Generated %d bytes in decomp tables\n", $bytes_out;
}

sub print_line_break
{
    my ($last) = @_;
    my ($outfile) = "gunibreak.h";

    local ($bytes_out) = 0;

    print "Writing $outfile...\n";

    open (OUT, "> $outfile");

    print OUT "/* This file is automatically generated.  DO NOT EDIT!\n";
    print OUT "   Instead, edit gen-unicode-tables.pl and re-run.  */\n\n";

    print OUT "#ifndef BREAKTABLES_H\n";
    print OUT "#define BREAKTABLES_H\n\n";

    print OUT "#define G_UNICODE_DATA_VERSION \"$ARGV[0]\"\n\n";

    printf OUT "#define G_UNICODE_LAST_CHAR 0x%04x\n\n", $last;

    printf OUT "#define G_UNICODE_MAX_TABLE_INDEX 1000\n\n";

    $table_index = 0;
    printf OUT "static const char break_property_data[][256] = {\n";
    for ($count = 0; $count <= $last; $count += 256)
    {
	$row[$count / 256] = &print_row ($count, 1, \&fetch_break_type);
    }
    printf OUT "\n};\n\n";

    print OUT "static const short break_property_table[256] = {\n";
    for ($count = 0; $count <= $last; $count += 256)
    {
	print OUT ",\n" if $count > 0;
	print OUT "  ", $row[$count / 256];
	$bytes_out += 2;
    }
    print OUT "\n};\n\n";

    print OUT "#endif /* BREAKTABLES_H */\n";

    close (OUT);

    printf STDERR "Generated %d bytes in break tables\n", $bytes_out;
}


# A fetch function for the break properties table.
sub fetch_break_type
{
    my ($index) = @_;
    return $break_mappings{$break_props[$index]};
}

# Fetcher for combining class.
sub fetch_cclass
{
    my ($i) = @_;
    return $cclass[$i];
}

# Expand a character decomposition recursively.
sub expand_decomp
{
    my ($code, $compat) = @_;

    my ($iter, $val);
    my (@result) = ();
    foreach $iter (split (' ', $decompositions[$code]))
    {
	$val = hex ($iter);
	if (defined $decompositions[$val] && 
	    ($compat || !$decompose_compat[$val]))
	{
	    push (@result, &expand_decomp ($val, $compat));
	}
	else
	{
	    push (@result, $val);
	}
    }

    return @result;
}

sub make_decomp
{
    my ($code, $compat) = @_;

    my $result = "";
    foreach $iter (&expand_decomp ($code, $compat))
    {
	$result .= sprintf "\\x%02x\\x%02x", $iter / 256, $iter & 0xff;
    }

    $result;
}
# Generate special case data string from two fields
sub add_special_case
{
    my ($code, $single, $field1, $field2) = @_;

    @values = (defined $single ? $single : (),
	       (map { hex ($_) } split /\s+/, $field1),
               0,
               (map { hex ($_) } split /\s+/, $field2));
    $result = "";


    for $value (@values) {
	$result .= sprintf ("\\x%02x\\x%02x", $value / 256, $value & 0xff);
    }

    $result .= "\\0";
    
    if (2 * @values + 2 > $special_case_len) {
	$special_case_len = 2 * @values + 2;
    }

    push @special_cases, $result;

    #
    # We encode special cases in the surrogate pair space
    #
    $value[$code] = 0xD800 + scalar(@special_cases) - 1;
}

sub output_special_case_table
{
    my $out = shift;

    print $out <<EOT;

/* Table of special cases for case conversion; each record contains
 * First, the best single character mapping to lowercase if Lu, 
 * and to uppercase if Ll, followed by the output mapping for the two cases 
 * other than the case of the codepoint, in the order [Ll],[Lu],[Lt],
 * separated and terminated by a double NUL.
 */
static const guchar special_case_table[][$special_case_len] = {
EOT

    for $case (@special_cases) {
	print $out qq( "$case",\n);
    }

    print $out <<EOT;
};

EOT

    print STDERR "Generated ", ($special_case_len * scalar @special_cases), " bytes in special case table\n";
}

sub enumerate_ordered
{
    my ($array) = @_;

    my $n = 0;
    for my $code (sort { $a <=> $b } keys %$array) {
	if ($array->{$code} == 1) {
	    delete $array->{$code};
	    next;
	}
	$array->{$code} = $n++;
    }

    return $n;
}

sub output_composition_table
{
    print STDERR "Generating composition table\n";
    
    local ($bytes_out) = 0;

    my %first;
    my %second;

    # First we need to go through and remove decompositions
    # starting with a non-starter, and single-character 
    # decompositions. At the same time, record
    # the first and second character of each decomposition
    
    for $code (keys %compositions) {
	@values = map { hex ($_) } split /\s+/, $compositions{$code};
	if ($cclass[$values[0]]) {
	    delete $compositions{$code};
	    next;
	}
	if (@values == 1) {
	    delete $compositions{$code};
	    next;
	}
	if (@values != 2) {
	    die "$code has more than two elements in its decomposition!\n";
	}

	if (exists $first{$values[0]}) {
	    $first{$values[0]}++;
	} else {
	    $first{$values[0]} = 1;
	}
    }

    # Assign integer indicices, removing singletons
    my $n_first = enumerate_ordered (\%first);

    # Now record the second character if each (non-singleton) decomposition
    for $code (keys %compositions) {
	@values = map { hex ($_) } split /\s+/, $compositions{$code};

	if (exists $first{$values[0]}) {
	    if (exists $second{$values[1]}) {
		$second{$values[1]}++;
	    } else {
		$second{$values[1]} = 1;
	    }
	}
    }

    # Assign integer indices, removing duplicate
    my $n_second = enumerate_ordered (\%second);

    # Build reverse table

    my @first_singletons;
    my @second_singletons;
    my %reverse;
    for $code (keys %compositions) {
	@values = map { hex ($_) } split /\s+/, $compositions{$code};

	my $first = $first{$values[0]};
	my $second = $second{$values[1]};

	if (defined $first && defined $second) {
	    $reverse{"$first|$second"} = $code;
	} elsif (!defined $first) {
	    push @first_singletons, [ $values[0], $values[1], $code ];
	} else {
	    push @second_singletons, [ $values[1], $values[0], $code ];
	}
    }

    @first_singletons = sort { $a->[0] <=> $b->[0] } @first_singletons;
    @second_singletons = sort { $a->[0] <=> $b->[0] } @second_singletons;

    my %vals;
    
    open OUT, ">gunicomp.h" or die "Cannot open gunicomp.h: $!\n";
    
    # Assign values in lookup table for all code points involved
    
    my $total = 1;
    my $last = 0;
    printf OUT "#define COMPOSE_FIRST_START %d\n", $total;
    for $code (keys %first) {
	$vals{$code} = $first{$code} + $total;
	$last = $code if $code > $last;
    }
    $total += $n_first;
    $i = 0;
    printf OUT "#define COMPOSE_FIRST_SINGLE_START %d\n", $total;
    for $record (@first_singletons) {
	my $code = $record->[0];
	$vals{$code} = $i++ + $total;
	$last = $code if $code > $last;
    }
    $total += @first_singletons;
    printf OUT "#define COMPOSE_SECOND_START %d\n", $total;
    for $code (keys %second) {
	$vals{$code} = $second{$code} + $total;
	$last = $code if $code > $last;
    }
    $total += $n_second;
    $i = 0;
    printf OUT "#define COMPOSE_SECOND_SINGLE_START %d\n\n", $total;
    for $record (@second_singletons) {
	my $code = $record->[0];
	$vals{$code} = $i++ + $total;
	$last = $code if $code > $last;
    }

    # Output lookup table

    my @row;						  
    $table_index = 0;
    printf OUT "static const gushort compose_data[][256] = {\n";
    for (my $count = 0; $count <= $last; $count += 256)
    {
	$row[$count / 256] = &print_row ($count, 2, sub { exists $vals{$_[0]} ? $vals{$_[0]} : 0; });
    }
    printf OUT "\n};\n\n";

    print OUT "static const short compose_table[256] = {\n";
    for (my $count = 0; $count <= $last; $count += 256)
    {
	print OUT ",\n" if $count > 0;
	print OUT "  ", $row[$count / 256];
	$bytes_out += 4;
    }
    print OUT "\n};\n\n";

    # Output first singletons

    print OUT "static const gushort compose_first_single[][2] = {\n";
    $i = 0;				     
    for $record (@first_singletons) {
	print OUT ",\n" if $i++ > 0;
	printf OUT " { %#06x, %#06x }", $record->[1], $record->[2];
    }
    print OUT "\n};\n";
				     
    $bytes_out += @first_singletons * 4;				     
		  
    # Output second singletons

    print OUT "static const gushort compose_second_single[][2] = {\n";
    $i = 0;				     
    for $record (@second_singletons) {
	print OUT ",\n" if $i++ > 0;
	printf OUT " { %#06x, %#06x }", $record->[1], $record->[2];
    }
    print OUT "\n};\n";
				     
    $bytes_out += @second_singletons * 4;				     
		  
    # Output array of composition pairs

    print OUT <<EOT;
static const gushort compose_array[$n_first][$n_second] = {
EOT
			
    for (my $i = 0; $i < $n_first; $i++) {
	print OUT ",\n" if $i;
	print OUT " { ";
	for (my $j = 0; $j < $n_second; $j++) {
	    print OUT ", " if $j;
	    if (exists $reverse{"$i|$j"}) {
		printf OUT "%#06x", $reverse{"$i|$j"};
	    } else {
		print OUT "     0";
            }
	}
	print OUT " }";
    }
    print OUT "\n";

    print OUT <<EOT;
};
EOT

    $bytes_out += $n_first * $n_second * 2;
    
    printf STDERR "Generated %d bytes in compose tables\n", $bytes_out;
}

sub output_casefold_table
{
    my $out = shift;

    print $out <<EOT;

/* Table of casefolding cases that can't be derived by lowercasing
 */
static const struct {
  guint16 ch;
  gchar data[$casefoldlen];
} casefold_table[] = {
EOT

   @casefold = sort { $a->[0] <=> $b->[0] } @casefold; 
    
   for $case (@casefold) {
       $code = $case->[0];
       $string = $case->[1];
       print $out sprintf(qq({ %#04x, "$string" },\n), $code);
    
   }

    print $out <<EOT;
};

EOT

   my $recordlen = (2+$casefoldlen+1) & ~1;
   printf "Generated %d bytes for casefold table\n", $recordlen * @casefold;
}

			     


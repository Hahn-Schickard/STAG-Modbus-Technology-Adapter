# Doxygen special command cheatsheet

This list is based on [star.bnl.gov](https://www.star.bnl.gov/public/comp/sofi/doxygen/commands.html#cmdauthor) list. It has been curated down to most relevant commands for code documentation. For full list please visit the previously mention website or [doxygen.nl](http://www.doxygen.nl/manual/commands.html) for more information. 

All commands in the documentation start with a backslash (\) or an at-sign (@). If you prefer you can replace all commands starting with a backslash below, by their counterparts that start with an at-sign.

Some commands have one or more arguments. Each argument has a certain range:

    If <sharp> braces are used the argument is a single word.
    If (round) braces are used the argument extends until the end of the line on which the command was found.
    If {curly} braces are used the argument extends until the next paragraph. Paragraphs are delimited by a blank line or by a section indicator.
    
If [square] brackets are used the argument is optional.

## Command list

    \@a <word> 		                Displays the argument <word> in italics. <br>  
    \@arg { item-description } 	    Describes a simple not nested argument. <br>
    \@attention { text }		    Starts a paragraph where a message that needs attention may be entered. <br>
    \@author { list of authors }    Starts a paragraph where one or more author names may be entered. <br>
    \@b <word> 					    Displays the argument <word> using a bold font. <br>
    \@brief { text }			    Starts a paragraph that serves as a brief description. <br>
    \@bug { bug description }	    Starts a paragraph where one or more bugs may be reported. <br>  
    \@c <word>					    Displays the argument <word> using a typewriter font. <br>
    \@code [ '{'<word>'}']		    Starts a code block. Everything in this block is treated as source code, and proper highlights will be used to showthe-content.    losed of with \endcode <br>
    \@date { date description }	    Starts a paragraph where one or more dates may be entered. <br>
    \@deprecated { description }    Starts a paragraph indicating that this documentation block belongs to a deprecated entity. <br>
    \@e <word>					    Displays the argument <word> in italics. <br>
    \@exception <exc> { text }	    Starts an exception description for an exception object with name <exc> <br>
    \@f$ <formulae> \f$			    Marks an inline formulae. <br>
    \@f[ <formulae> \f]			    Marks a long center lined formulae. <br>
    \@invariant { text }		    Starts a paragraph where the invariant of an entity can be described. <br>
    \@li { item-description }	    Marks a single element of an unordered list. <br>
    \@note { text }				    Starts a paragraph where a note can be entered. <br>  
    \@param <par> { text }		    Starts a parameter description for a function parameter with name <par>. <br>
    \@post { text }				    Starts a paragraph where the postcondition of an entity can be described. <br>
    \@pre { text }				    Starts a paragraph where the precondition of an entity can be described. <br>
    \@remarks { text }			    Starts a paragraph where one or more remarks may be entered. <br>
    \@return { text }			    Starts a return value description for a function. <br>
    \@retval <value> { text }	    Starts a paragraph where return type with name <value> can be entered. <br>
    \@since { text }			    This tag can be used to specify since when (version or time) an entity is available. <br>
    \@test { text }				    Starts a paragraph where a test case can be described. <br>
    \@todo { text }				    Starts a paragraph, where notes for future work may be entered. <br>
    \@version { version number }    Starts a paragraph where one or more version strings may be entered. <br>
    \@warning { text }			    Starts a paragraph where one or more warning messages may be entered. <br>
    \@~[LanguageId]				    Specifies language translation options. <br>
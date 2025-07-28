package jxa;

import java.util.List;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class Jxa
{
	public static List<String> _posArguments = new ArrayList<>();
	
	/* Saves the position where a certain short-name
	 * can be found within 'flags' array
	 */
	private static final int[] _quickShortNames = new int[26 + 26 + 10];
	
	/* Saves the position where a certain long-name
	 * can be found within 'flags' array
	 */
	private static final Map<String, Integer> _quickLongNames = new HashMap<>();
	
	/* This variable is only assigned to some actual flag
	 * when the flag needs is different from JxaFlag.FlagArg.NON
	 */
	private static JxaFlag<?> _thisFlag = null;
	
	public static void parse (JxaFlag<?> flags[], final String args[])
	{
		_checkCorrectness(flags);
		
		boolean argends = false;
		for (int i = 0; i < args.length; i++)
		{
			final String arg = args[i];
			
			if (argends)
			{
				_posArguments.add(arg);
				continue;
			}
			/* Once '--' is found anything that comes after
			 * will be treated as a positional argument (UNIX standard)
			 */
			else if (arg.equals("--"))
			{
				_makeSureFlagHasItsArg();
				argends = true;
				continue;
			}
			/* In UNIX when a single dash is found, the program
			 * will have to read from STDIN and treat it as an argument
			 */
			else if (arg.equals("-"))
			{
				JxaFatal.unsupportedStdin();	
			}
			else if (arg.startsWith("--"))
			{
				_makeSureFlagHasItsArg();
				_handleLong(arg, flags);
			}
			else if (arg.startsWith("-"))
			{
				_makeSureFlagHasItsArg();
				_handleShort(arg, flags);
			}
			/* A 'free word' means anything that is not a flag
			 * for example: 54 "hello java" etc (constant values)
			 */
			else
			{
				_handleFreeWord(arg);
			}
		}
		_makeSureFlagHasItsArg();
	}	
	
	private static void _checkCorrectness (final JxaFlag<?>[] flags)
	{
		for (int i = 0; i < flags.length; i++)
		{
			final char id = flags[i].getShortname();
			final int key = _getIdKey(id);
			
			if (key == -1)
			{
				JxaFatal.invalidId(flags[i]);
			}
			if (_quickShortNames[key] != 0)
			{
				JxaFatal.duplicatedId(_quickShortNames[key] - 1, i, flags);
			}
			
			final String longname = flags[i].getLongname();
			if (_quickLongNames.containsKey(longname))
			{
				JxaFatal.duplicatedName(flags[i]);
			}
			
			_quickShortNames[key] = i + 1;	
			_quickLongNames.put(longname, i);
		}
	}
	
	private static int _getIdKey (char shortName)
	{
		if (Character.isDigit(shortName))     { return shortName - '0'; }
		if (Character.isLowerCase(shortName)) { return shortName - 'a' + 10; }
		if (Character.isUpperCase(shortName)) { return shortName - 'A' + 36; }
		
		return -1;
	}

	private static void _handleLong (String longName, JxaFlag<?>[] flags)
	{
		String rmDashes = longName.substring(2);
		String arg = null;
		
		if (rmDashes.contains("="))
		{
			final int eqIndex = rmDashes.indexOf('=');
			arg = (String) rmDashes.subSequence(eqIndex + 1, rmDashes.length());
			rmDashes = rmDashes.substring(0, eqIndex);	
		}
		
		final int at = _quickLongNames.getOrDefault(rmDashes, -1);
		if (at == -1) { JxaFatal.undefinedFlag(rmDashes); }
		
		_thisFlag = flags[at];
		_thisFlag.setSeen2True();

		if (_thisFlag.getArgTaker() == JxaFlag.arg.non)
		{
			if (arg != null) { _posArguments.add(arg); }
			_thisFlag = null;
		}
		else if (arg != null)
		{
			_thisFlag.setArgument(arg);
			_thisFlag = null;
		}
	}
	
	private static void _handleShort (String arg, JxaFlag<?>[] flags)
	{
		boolean theresOneWhichTakesArgAlready = false;
		char firstTakingArg = 0;
		
		for (int i = 1; i < arg.length(); i++)
		{
			final char thisId =  arg.charAt(i);
			final int key = _getIdKey(thisId);
			
			if (key == -1) { JxaFatal.undefinedFlag(thisId); }
			final int locatedAt = _quickShortNames[key];
			
			if (locatedAt == 0) { JxaFatal.undefinedFlag(thisId); }
			JxaFlag<?> flag = flags[locatedAt - 1];
			
			flag.setSeen2True();
			
			if (flag.getArgTaker() != JxaFlag.arg.non && theresOneWhichTakesArgAlready)
			{	
				JxaFatal.groupedArgedFlags(arg, firstTakingArg, thisId);
			}
			
			if (flag.getArgTaker() != JxaFlag.arg.non)
			{
				theresOneWhichTakesArgAlready = true;
				firstTakingArg = thisId;
				_thisFlag = flag;
			}
		}
	}
	
	private static void _handleFreeWord (String given)
	{
		if (_thisFlag != null) { _thisFlag.setArgument(given); _thisFlag = null; }	
		else { _posArguments.add(given); }
	}		
	
	private static void _makeSureFlagHasItsArg ()
	{
		if (_thisFlag != null && _thisFlag.getArgTaker() == JxaFlag.arg.yes && !_thisFlag.wasArgGiven())
		{
			JxaFatal.missingArgument(_thisFlag);
		}
	}
}
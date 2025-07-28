package jxa;

import java.util.function.BiConsumer;
import java.util.function.Function;

public class JxaFlag<T>
{	
	public enum arg
	{
		non,
		may,
		yes
	};
	
	final private String longname;
	final private String description;
	final private char   shortname;
	final private arg    argtaker;

	private T ifails_v = null;
	private BiConsumer<JxaFlag<?>, String> ifails_f = JxaFatal::mismatchedTypes;

	private T argument;
	private boolean seen = false;
	private boolean argiven = false;

	public JxaFlag (final String longname, final String description, final char shortname, final arg argtaker)
	{
		this.longname = longname;
		this.description = description;
		this.shortname = shortname;
		this.argtaker = argtaker;
	}

	public JxaFlag (final String longname, final String description, final char shortname, final arg argtaker, final T defa)
	{
		this.longname = longname;
		this.description = description;
		this.shortname = shortname;
		this.argtaker = argtaker;
		this.argument = defa;
	}
	
	public JxaFlag (final String longname, final String description, final char shortname, final arg argtaker, final T defa, final T ifails)
	{
		this.longname = longname;
		this.description = description;
		this.shortname = shortname;
		this.argtaker = argtaker;
		this.argument = defa;
		this.ifails_v = ifails;
	}

	public JxaFlag (final String longname, final String description, final char shortname, final arg argtaker, final T defa, final BiConsumer<JxaFlag<?>, String> ifails)
	{
		this.longname = longname;
		this.description = description;
		this.shortname = shortname;
		this.argtaker = argtaker;
		this.argument = defa;
		this.ifails_f = ifails;
	}
	
	public String getLongname ()    { return this.longname; }
	public String getDescription () { return this.description; }
	public char   getShortname ()   { return this.shortname; }
	public arg    getArgTaker ()    { return this.argtaker; }
	public T      getArgument ()    { return this.argument; }
	public T      getIfailsV ()     { return this.ifails_v; }
	public boolean wasSeen ()       { return this.seen; }
	public boolean wasArgGiven ()   { return this.argiven; }

	public BiConsumer<JxaFlag<?>, String> getIfailsF () { return this.ifails_f; }
	
	@SuppressWarnings("unchecked")
	public void setArgument (final String as)
	{
		final Class<T> is = (Class<T>) this.argument.getClass();
		if (is == String.class)
		{
			this.argument = (T) as;
			this.argiven = true;
			return;
		}		
		
		Function<String, ?> caster = Byte::valueOf;
		
		if (is == Short.class)       { caster = Short::valueOf; }
		if (is == Integer.class)     { caster = Integer::valueOf; }
		else if (is == Long.class)   { caster = Long::valueOf; }
		else if (is == Float.class)  { caster = Float::valueOf; }	
		else if (is == Double.class) { caster = Double::valueOf; }
		else { throw new IllegalArgumentException("PROGRAMMER: invalid flag's type provided: " + is.toString()); }

		try
		{
			this.argument = (T) caster.apply(as);
			this.argiven = true;
		}
		catch (NumberFormatException e)
		{	
			if (this.ifails_v == null) { this.ifails_f.accept(this, as); }
			else { this.argument = this.ifails_v; this.argiven = true; }
		}
	}

	public void setSeen2True ()
	{
		this.seen = true;
	}
}
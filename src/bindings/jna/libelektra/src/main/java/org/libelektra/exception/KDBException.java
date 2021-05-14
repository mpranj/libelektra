package org.libelektra.exception;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import org.libelektra.Key;


/**
 * This exception wraps Elektra errors into the corresponding Java Exceptions
 */
public abstract class KDBException extends Exception
{
	protected static final String META_KEY_NOT_FOUND_VALUE = "(unknown)";

	private static final long serialVersionUID = 1L;

	private final transient Key errorKey;
	private final transient List<WarningEntry> warnings;

	/**
	 * KDBException which holds the errorKey
	 * @param k The errorKey
	 */
	protected KDBException (final Key k)
	{
		errorKey = k;
		warnings = new ArrayList<> ();

		Optional<String> oWarningsKeyValue = k.getMeta ("warnings").map (Key::getString);
		if (oWarningsKeyValue.isEmpty ())
		{
			return;
		}
		final String lastArrayIndex = oWarningsKeyValue.get ();
		final int arraySize = Integer.valueOf (lastArrayIndex.replaceAll ("^#_*", ""));

		for (int i = 0; i <= arraySize; i++)
		{
			warnings.add (new WarningEntry (k, i));
		}
	}

	/**
	 * Gets the errorKey from Elektra
	 *
	 * @return ErrorKey from Elektra
	 */
	public Key getErrorKey ()
	{
		return errorKey;
	}

	/**
	 * Gets the errorNumber from Elektra
	 *
	 * @return ErrorNumber from Elektra
	 */
	public String getErrorNumber ()
	{
		return errorKey.getMeta ("error/number").map (Key::getString).orElse (META_KEY_NOT_FOUND_VALUE);
	}

	/**
	 * Returns the affected configuration file of the error.
	 * It empty returns the parents Key name
	 *
	 * @return either the configuration file or if empty the parent key name
	 */
	public String getConfigFile ()
	{
		return errorKey.getMeta ("error/configfile").map (Key::getString).filter (s -> !s.isEmpty ()).orElseGet (errorKey::getName);
	}

	/**
	 * Returns the mountpoint of the configuration
	 *
	 * @return the mountpoint of the configuration
	 */
	public String getMountpoint ()
	{
		return errorKey.getMeta ("error/mountpoint").map (Key::getString).orElse (META_KEY_NOT_FOUND_VALUE);
	}

	/**
	 * Prints Elektra specific debug information in the form of "At: file:line"
	 *
	 * @return Elektra specific debug information in the form of "At: file:line"
	 */
	public String getDebugInformation ()
	{
		return String.format ("At: %s:%s", errorKey.getMeta ("error/file").map (Key::getString).orElse (META_KEY_NOT_FOUND_VALUE),
				      errorKey.getMeta ("error/line").map (Key::getString).orElse (META_KEY_NOT_FOUND_VALUE));
	}

	/**
	 * Returns the module which has thrown the error
	 *
	 * @return the module which has thrown the error
	 */
	public String getModule ()
	{
		return errorKey.getMeta ("error/module").map (Key::getString).orElse (META_KEY_NOT_FOUND_VALUE);
	}

	/**
	 * @see #getMessage()
	 */
	@Override public String getLocalizedMessage ()
	{
		return getMessage ();
	}

	/**
	 * Returns the error reason which is written to the `error/reason` metakey of the errorkey
	 * @return The reason for the error
	 */
	public String getReason ()
	{
		return errorKey.getMeta ("error/reason").map (Key::getString).orElse (META_KEY_NOT_FOUND_VALUE);
	}

	/**
	 * getMessage() returns the thrown Elektra error in the same format as it would be printed in the terminal
	 * @return The complete error information in a String with configfile, moutpoint and debuginformation
	 */
	@Override public String getMessage ()
	{
		StringBuilder builder = new StringBuilder ();
		builder.append (String.format ("Sorry, module %s issued error %s:", getModule (), getErrorNumber ())).append ("\n");
		builder.append (getReason ()).append ("\n");
		builder.append ("Configfile: ").append (getConfigFile ()).append ("\n");
		if (errorKey.getMeta ("error/mountpoint") != null)
		{
			builder.append ("Mountpoint: ").append (getMountpoint ()).append ("\n");
		}
		if (errorKey.getMeta ("error/file") != null)
		{
			builder.append (getDebugInformation ()).append ("\n");
		}
		return builder.toString ();
	}

	/**
	 * If an error occurred it may also has important warnings which caused the error.
	 * This method checks if they are available
	 *
	 * @return true if additional warnings were emitted
	 */
	public boolean hasWarnings ()
	{
		return !warnings.isEmpty ();
	}

	/**
	 * Returns the warnings list
	 *
	 * @return the warnings list
	 */
	public List<WarningEntry> getWarnings ()
	{
		return warnings;
	}
}

package org.opencyc.cycobject;

import java.io.*;
import org.opencyc.xml.XMLWriter;

/**
 * This class implements a Cyc Fort (First Order Reified Term).
 *
 * @version $Id: CycFort.java,v 1.13 2002/05/30 20:00:27 stephenreed Exp $
 * @author Stephen L. Reed
 *
 * <p>Copyright 2001 Cycorp, Inc., license is open source GNU LGPL.
 * <p><a href="http://www.opencyc.org/license.txt">the license</a>
 * <p><a href="http://www.opencyc.org">www.opencyc.org</a>
 * <p><a href="http://www.sourceforge.net/projects/opencyc">OpenCyc at SourceForge</a>
 * <p>
 * THIS SOFTWARE AND KNOWLEDGE BASE CONTENT ARE PROVIDED ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OPENCYC
 * ORGANIZATION OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE AND KNOWLEDGE
 * BASE CONTENT, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
public abstract class CycFort extends CycObject implements Serializable, Comparable {


    /**
     * The name of the XML tag for id objects
     */
    public static final String idXMLTag = "id";

    /**
     * The ID of the <tt>CycFort<tt> object which is an integer unique within an OpenCyc
     * KB but not necessarily unique globally.
     */
    private Integer id;

    /**
     * Returns a cyclified string representation of the OpenCyc FORT.
     * Embedded constants are prefixed with ""#$".
     *
     * @return a cyclified <tt>String</tt>.
     */
    public abstract String cyclify();

    /**
     * Prints the XML representation of the CycFort to an <tt>XMLWriter</tt>
     */
    public abstract void toXML (XMLWriter xmlWriter, int indent, boolean relative)
        throws IOException;

    /**
     * Returns this object in a form suitable for use as an <tt>String</tt> api expression value.
     *
     * @return this object in a form suitable for use as an <tt>String</tt> api expression value
     */
    public abstract String stringApiValue();

    /**
     * Returns this object in a form suitable for use as an <tt>CycList</tt> api expression value.
     *
     * @return this object in a form suitable for use as an <tt>CycList</tt> api expression value
     */
    public abstract Object cycListApiValue();

    /**
     * Sets the id.
     *
     * @param id the id value
     */
    public void setId(Integer id) {
        this.id = id;
    }

    /**
     * Gets the id.
     *
     * @return the id
     */
    public Integer getId() {
        return id;
    }

    /**
     * Returns a string representation without causing additional api calls to determine
     * constant names.
     *
     * @return a string representation without causing additional api calls to determine
     * constant names
     */
    public abstract String safeToString ();

    /**
     * Compares this object with the specified object for order.
     * Returns a negative integer, zero, or a positive integer as this
     * object is less than, equal to, or greater than the specified object.
     *
     * @param object the reference object with which to compare.
     * @return a negative integer, zero, or a positive integer as this
     * object is less than, equal to, or greater than the specified object
     */
     public int compareTo (Object object) {
        if (this instanceof CycConstant) {
            if (object instanceof CycConstant)
                return this.toString().compareTo(object.toString());
            else if (object instanceof CycNart)
                return this.toString().compareTo(object.toString().substring(1));
            else
                throw new ClassCastException("Must be a CycFort object");
        }
        else {
            if (object instanceof CycNart)
                return this.toString().compareTo(object.toString());
            else if (object instanceof CycConstant)
                return this.toString().substring(1).compareTo(object.toString());
            else
                throw new ClassCastException("Must be a CycFort object");
        }
     }

}


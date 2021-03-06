/*******************************************************************************
 * Copyright (c) 2014-2015 Zeligsoft (2009) Limited and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *******************************************************************************/
package org.eclipse.papyrusrt.codegen.lang.cpp.expr;

import org.eclipse.papyrusrt.codegen.lang.cpp.Expression;
import org.eclipse.papyrusrt.codegen.lang.cpp.Name;
import org.eclipse.papyrusrt.codegen.lang.cpp.Type;
import org.eclipse.papyrusrt.codegen.lang.cpp.dep.DependencyList;
import org.eclipse.papyrusrt.codegen.lang.cpp.dep.ElementDependency;
import org.eclipse.papyrusrt.codegen.lang.cpp.element.NamedElement;
import org.eclipse.papyrusrt.codegen.lang.cpp.internal.CppFormatter;

public class ElementAccess extends Expression
{
    protected final NamedElement element;

    public ElementAccess( NamedElement element )
    {
        this.element = element;
    }

    public Name getName() { return element.getName(); }

    @Override
    protected Type createType()
    {
        return element.getType();
    }

    @Override public Precedence getPrecedence() { return Precedence.Default; }

    @Override
    public boolean addDependencies( DependencyList deps )
    {
        return deps.add( new ElementDependency( element ) )
            && element.getType().addDependencies( deps );
    }

    @Override
    public boolean write( CppFormatter fmt )
    {
        // Access expressions should not use the qualified name.
        return fmt.write( getName().getIdentifier() );
    }
}

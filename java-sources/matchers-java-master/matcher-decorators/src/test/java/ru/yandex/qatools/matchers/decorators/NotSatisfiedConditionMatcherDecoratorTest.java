package ru.yandex.qatools.matchers.decorators;

import org.hamcrest.Matcher;
import org.junit.Rule;
import org.junit.Test;
import org.junit.internal.AssumptionViolatedException;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runner.RunWith;
import org.junit.runners.model.Statement;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import static org.junit.Assert.assertThat;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.when;
import static ru.yandex.qatools.matchers.decorators.MatcherDecoratorsBuilder.should;

/**
 * @author Alexander Tolmachev starlight@yandex-team.ru
 *         Date: 06.03.13
 */
@RunWith(MockitoJUnitRunner.class)
public class NotSatisfiedConditionMatcherDecoratorTest {
    @Mock
    private Matcher<Object> condition;

    private final Object itemToMatchCondition = new Object();

    @Mock
    private Matcher<Object> matcher;

    private final Object arbitraryObject = new Object();

    @Rule
    public TestRule testRule = new TestRule() {
        @Override
        public Statement apply(final Statement base, Description description) {
            return new Statement() {
                @Override
                public void evaluate() throws Throwable {
                    try {
                        base.evaluate();
                        fail();
                    } catch (AssumptionViolatedException e) {
                        // Skip
                    }
                }
            };
        }
    };

    @Test
    public void notSatisfiedConditionShouldCauseAssumptionViolatedException() {
        when(condition.matches(itemToMatchCondition)).thenReturn(false);

        assertThat(arbitraryObject, should(matcher).inCase(itemToMatchCondition, condition));
    }
}

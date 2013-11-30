package ru.yandex.qatools.matchers.webdriver;

import org.hamcrest.Description;
import org.hamcrest.Factory;
import org.hamcrest.TypeSafeMatcher;
import org.openqa.selenium.WebDriverException;
import org.openqa.selenium.WebElement;

/**
 * @author Artem Eroshenko eroshenkoam
 *         4/23/13, 11:15 PM
 */
public class DisplayedMatcher extends TypeSafeMatcher<WebElement> {

    @Override
    protected boolean matchesSafely(WebElement element) {
        try {
            return element.isDisplayed();
        } catch (WebDriverException e) {
            return false;
        }
    }

    public void describeTo(Description description) {
        description.appendText("element is displayed on page");
    }

    @Override
    public void describeMismatchSafely(WebElement element, Description mismatchDescription) {
        mismatchDescription.appendValue(element).
                appendText(" is not displayed on page");
    }

    @Factory
    public static DisplayedMatcher displayed() {
        return new DisplayedMatcher();
    }

}
